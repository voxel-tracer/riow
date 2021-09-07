#include "bvh.h"

#include "bvh_structs.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include <yocto/yocto_sceneio.h>

const int nBuckets = 12;

// BVHAccel Local Declarations
struct BVHPrimitiveInfo {
    BVHPrimitiveInfo() : primitiveNumber(0) {}
    BVHPrimitiveInfo(size_t primitiveNumber, const yocto::bbox3f& bounds)
        : primitiveNumber(primitiveNumber), bounds(bounds), centroid(.5f * bounds.min + .5f * bounds.max) { }
    size_t primitiveNumber;
    yocto::bbox3f bounds;
    yocto::vec3f centroid;
};

struct BVHBuildNode {
    void InitLeaf(int first, int n, const yocto::bbox3f& b) {
        firstPrimOffset = first;
        nPrimitives = n;
        bounds = b;
        children[0] = children[1] = nullptr;
    }

    void InitInterior(int axis, BVHBuildNode* c0, BVHBuildNode* c1) {
        children[0] = c0;
        children[1] = c1;
        bounds = yocto::merge(c0->bounds, c1->bounds);
        splitAxis = axis;
        nPrimitives = 0;
    }

    yocto::bbox3f bounds;
    BVHBuildNode* children[2];
    uint32_t splitAxis, firstPrimOffset, nPrimitives;
};

void BVHAccel::computeQuality(const BVHBuildNode* node, float rootSA, float *largestOverlap) {
    if (node->nPrimitives == 0) {
        BVHBuildNode* left = node->children[0];
        BVHBuildNode* right = node->children[1];
        // compute overlap of children nodes and the ratio of its surface area vs this node's surface area
        float qa = SurfaceArea(Intersect(left->bounds, right->bounds)) / rootSA;
        if (qa > *largestOverlap)
            *largestOverlap = qa;

        computeQuality(node->children[0], rootSA, largestOverlap);
        computeQuality(node->children[1], rootSA, largestOverlap);
    }
}

BVHAccel::BVHAccel(const yocto::scene_shape& shape, SplitMethod splitMethod, int maxPrimsInNode, float internalCost, 
        bool reevaluateCost, bool useBestDim) : maxPrimsInNode(maxPrimsInNode), splitMethod(splitMethod),
        shape(shape), internalCost(internalCost), reevaluateCost(reevaluateCost), useBestDim(useBestDim) {
    if (shape.triangles.empty()) return;

    // Initialize primitiveInfo array from shape.triangles
    std::vector<BVHPrimitiveInfo> primitives(shape.triangles.size());
    for (auto i : yocto::range(shape.triangles.size())) {
        const auto& t = shape.triangles[i];
        primitives[i] = { i, yocto::triangle_bounds(shape.positions[t.x], shape.positions[t.y], shape.positions[t.z]) };
    }

    // Build BVH from primitives

    // Compute min overlap below which we no longer consider spatial split
    yocto::bbox3f bounds;
    for (int i = 0; i < primitives.size(); i++) {
        bounds = yocto::merge(bounds, primitives[i].bounds);
    }
    minOverlap = SurfaceArea(bounds) * 0.00001;

    // Build BVH tree for primitives using PrimitiveInfo
    int totalNodes = 0;
    int addedSplits = 0;
    elements.reserve(primitives.size());
    BVHBuildNode* root;
    // we only support SplitMethod::EqualCounts
    root = recursiveBuild(primitives, 0, primitives.size(), &totalNodes, &addedSplits);
    primitives.resize(0);
    // compute estimated node quality
    float largestOverlap = 0.0f;
    computeQuality(root, SurfaceArea(root->bounds), &largestOverlap);
    std::cerr << "BVH created with " << totalNodes << " nodes for " << shape.triangles.size() << " primitives\n";
    if (addedSplits > 0)
        std::cerr << "  Including " << addedSplits << " additional splits" << std::endl;
    std::cerr << "BVH largest overlap is " << largestOverlap << std::endl;
    if (convertedNodes > 0)
        std::cerr << convertedNodes << " nodes converted to leaves" << std::endl;
    if (trimmedNodes > 0)
        std::cerr << trimmedNodes << " nodes trimmed" << std::endl;

    // Compute representation of depth-first traversal of BVH tree
    nodes.resize(totalNodes);
    int offset = 1;
    flattenBVHTree(root, 0, &offset);
}

BVHAccel::~BVHAccel() { }

struct BucketInfo {
    int count = 0;
    yocto::bbox3f bounds;
};

struct SplitCandidate {
    int dim;
    int bucket;
    float cost;
    float overlap;
};

float BVHAccel::sahCost(const BVHBuildNode* node, float rootSA) const {
    float nodeSARatio = SurfaceArea(node->bounds) / rootSA;
    if (node->nPrimitives > 0) {
        return node->nPrimitives * nodeSARatio;
    }

    return internalCost * nodeSARatio + 
        sahCost(node->children[0], rootSA) + 
        sahCost(node->children[1], rootSA);
}

SplitCandidate findObjectSplit(std::vector<BVHPrimitiveInfo>& primitiveInfo, int start, int end, 
        const yocto::bbox3f& bounds, const yocto::bbox3f& centroidBounds, bool useBestDim, float internalCost) {
    SplitCandidate split;
    split.cost = yocto::flt_max;

    // Allocate BucketInfo for SAH partition buckets
    BucketInfo buckets[nBuckets];

    int minDim = 0;
    int maxDim = 2;
    if (useBestDim) {
        minDim = maxDim = MaximumExtent(centroidBounds);
    }

    for (int dim = minDim; dim <= maxDim; dim++) {
        // reset buckets
        for (int b = 0; b < nBuckets; b++) {
            buckets[b].bounds = {};
            buckets[b].count = 0;
        }

        // Initialize BucketInfo for SAH partition buckets
        for (int i = start; i < end; ++i) {
            int b = nBuckets * Offset(centroidBounds, primitiveInfo[i].centroid)[dim];
            if (b == nBuckets) b = nBuckets - 1;
            buckets[b].count++;
            buckets[b].bounds = merge(buckets[b].bounds, primitiveInfo[i].bounds);
        }

        // Compute costs for splitting after each bucket
        float cost[nBuckets - 1];
        for (int i = 0; i < nBuckets - 1; ++i) {
            yocto::bbox3f b0, b1;
            int count0 = 0, count1 = 0;
            for (int j = 0; j <= i; ++j) {
                b0 = merge(b0, buckets[j].bounds);
                count0 += buckets[j].count;
            }
            for (int j = i + 1; j < nBuckets; ++j) {
                b1 = merge(b1, buckets[j].bounds);
                count1 += buckets[j].count;
            }
            cost[i] = internalCost + (count0 * SurfaceArea(b0) + count1 * SurfaceArea(b1)) / SurfaceArea(bounds);
        }

        // Find bucket to split that minimizes SAH metric
        float minCost = cost[0];
        int minCostBucket = 0;
        for (int i = 1; i < nBuckets - 1; ++i) {
            if (cost[i] < minCost) {
                minCost = cost[i];
                minCostBucket = i;
            }
        }

        if (minCost < split.cost) {
            split.cost = minCost;
            split.dim = dim;
            split.bucket = minCostBucket;

            // compute overlap between object splits
            {
                yocto::bbox3f b0, b1;
                int count0 = 0, count1 = 0;
                for (int j = 0; j <= minCostBucket; ++j) {
                    b0 = merge(b0, buckets[j].bounds);
                }
                for (int j = minCostBucket + 1; j < nBuckets; ++j) {
                    b1 = merge(b1, buckets[j].bounds);
                }

                split.overlap = SurfaceArea(Intersect(b0, b1));
            }
        }
    }

    return split;
}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<BVHPrimitiveInfo>& primitiveInfo,
        int start, int end, int* totalNodes, int* addedSplits) {
    BVHBuildNode* node = new BVHBuildNode();
    (*totalNodes)++;
    // save this value as we may need it later when reevaluating SAH cost
    int firstPrimOffset = elements.size();
    int curTotalNodes = *totalNodes;
    // compute bounds of all primitives in BVH node
    yocto::bbox3f bounds;
    for (int i = start; i < end; i++)
        bounds = yocto::merge(bounds, primitiveInfo[i].bounds);
    int nPrimitives = end - start;
    if (nPrimitives == 1) {
        // create leaf BVHBuildNode
        for (int i = start; i < end; i++) {
            elements.push_back(primitiveInfo[i].primitiveNumber);
        }
        node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
        return node;
    } else {
        // Compute bound of primitive centroids, chose split dimension dim
        yocto::bbox3f centroidBounds;
        for (int i = start; i < end; i++)
            centroidBounds = yocto::merge(centroidBounds, primitiveInfo[i].centroid);
        int dim = MaximumExtent(centroidBounds);

        // Partition primitives into two sets and build children
        int mid = (start + end) / 2;
        if (centroidBounds.max[dim] == centroidBounds.min[dim]) {
            // create leaf BVHBuildNode
            for (int i = start; i < end; i++) {
                elements.push_back(primitiveInfo[i].primitiveNumber);
            }
            node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
            return node;
        } else {
            // Partition primitives based on SplitMethod
            switch (splitMethod) {
            case SplitMethod::EqualCounts: {
                // Partition primitives into equal sized subsets
                mid = (start + end) / 2;
                std::nth_element(&primitiveInfo[start], &primitiveInfo[mid], &primitiveInfo[end - 1] + 1,
                    [dim](const BVHPrimitiveInfo& a, const BVHPrimitiveInfo& b) {
                        return a.centroid[dim] < b.centroid[dim];
                    });
                break;
            }
            case SplitMethod::SAH: 
            default: {
                // Partition primitives using approximate SAH
                if (nPrimitives <= 2) {
                    // Partition primitives into equal sized subsets
                    mid = (start + end) / 2;
                    std::nth_element(&primitiveInfo[start], &primitiveInfo[mid], &primitiveInfo[end - 1] + 1,
                        [dim](const BVHPrimitiveInfo& a, const BVHPrimitiveInfo& b) {
                            return a.centroid[dim] < b.centroid[dim];
                        });
                } else {
                    SplitCandidate objectSplit = 
                        findObjectSplit(primitiveInfo, start, end, bounds, centroidBounds, useBestDim, internalCost);

                    // Either create leaf or split primitives at selected SAH bucket
                    float leafCost = nPrimitives;
                    float splitCost = objectSplit.cost;
                    if (nPrimitives <= maxPrimsInNode && leafCost <= splitCost) {
                        // Create leaf BVHBuildNode
                        for (int i = start; i < end; i++) {
                            elements.push_back(primitiveInfo[i].primitiveNumber);
                        }
                        node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
                        return node;
                    } else {
                        BVHPrimitiveInfo* pmid = std::partition(&primitiveInfo[start], &primitiveInfo[end - 1] + 1,
                            [=](const BVHPrimitiveInfo& pi) {
                                const int nBuckets = 12;
                                int b = nBuckets * Offset(centroidBounds, pi.centroid)[objectSplit.dim];
                                if (b == nBuckets) b = nBuckets - 1;
                                return b <= objectSplit.bucket;
                            });
                        mid = pmid - &primitiveInfo[0];
                    }
                }
                break;
            }
            }

            int leftSplits = 0;
            BVHBuildNode* child0 = recursiveBuild(primitiveInfo, start, mid, totalNodes, &leftSplits);
            int rightSplits = 0;
            BVHBuildNode* child1 = recursiveBuild(primitiveInfo, mid + leftSplits, end + leftSplits, totalNodes, &rightSplits);
            node->InitInterior(dim, child0, child1);
            *addedSplits += leftSplits + rightSplits;

            // TODO update reevaluateCost to allow resetting split primitives
            if (reevaluateCost && splitMethod == SplitMethod::SAH) {
                // reevaluate node splitting cost and create a leaf instead
                float splitCost = sahCost(node, SurfaceArea(node->bounds));
                float leafCost = nPrimitives;
                if (splitCost > leafCost) {
                    convertedNodes++;
                    trimmedNodes += (*totalNodes) - curTotalNodes;
                    // remove all intermediate nodes added so far
                    (*totalNodes) = curTotalNodes;
                    // primitives have already been written to primitiveInfo and orderedPrims
                    node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
                }
            }
        }
    }
    return node;
}

void BVHAccel::flattenBVHTree(BVHBuildNode* node, int offset, int* firstChildOffset) {
    // store node at offset
    // and its children, if any, at firstChildOffset and firstChildOffset+1
    LinearBVHNode* linearNode = &nodes[offset];
    linearNode->bounds = node->bounds;
    if (node->nPrimitives > 0) {
        linearNode->primitivesOffset = node->firstPrimOffset;
        linearNode->nPrimitives = node->nPrimitives;
    }
    else {
        // Create interior flattened BVH node
        linearNode->axis = node->splitAxis;
        linearNode->nPrimitives = 0;
        linearNode->firstChildOffset = (*firstChildOffset);
        (*firstChildOffset) += 2; // reserve space for both children
        flattenBVHTree(node->children[0], linearNode->firstChildOffset, firstChildOffset);
        flattenBVHTree(node->children[1], linearNode->firstChildOffset+1, firstChildOffset);

        // confirm that no two sibling nodes overlap
        //Bounds3 overlap = Intersect(node->children[0]->bounds, node->children[1]->bounds);
        //float maxOverlap = overlap.Diagonal()[overlap.MaximumExtent()];
        //if (maxOverlap > 0.00001f)
        //    std::cerr << "sibling nodes overlap with " << maxOverlap << std::endl;
    }
}

bool BVHAccel::hit(yocto::ray3f r, yocto::vec2f& uv, int& element, float& dist) const {
    yocto::vec3f invDir = 1.0f / r.d;
    yocto::vec3i dirIsNeg = { invDir[0] < 0, invDir[1] < 0, invDir[2] < 0 };

    // Follow ray through BVH nodes to find primitive intersections
    int toVisitOffset = 0, currentOffset = 0, currentNPrimitives = 0;
    int nodesToVisit[64 * 2];

    bool found = false; // true if at least one primitive intersected the ray

    // nodes[0] is automatically intersected
    currentOffset = 1;

    while (true) {
        if (currentNPrimitives > 0) {
            // Intersect ray with primitives in leaf BVH node
            for (auto idx = currentOffset; idx < currentOffset + currentNPrimitives; idx++) {
                const auto& t = shape.triangles[elements[idx]];
                if (intersect_triangle(r, shape.positions[t.x], shape.positions[t.y], shape.positions[t.z], uv, dist)) {
                    r.tmax = dist;
                    element = elements[idx];
                    found = true;
                }
            }
            if (toVisitOffset == 0) break;
            currentOffset = nodesToVisit[--toVisitOffset];
            currentNPrimitives = nodesToVisit[--toVisitOffset];
        }
        else {
            // load both children at once
            const LinearBVHNode& left  = nodes[currentOffset];
            const LinearBVHNode& right = nodes[currentOffset + 1];

            float leftDist, rightDist;
            bool traverseLeft = Hit(left.bounds, r, invDir, leftDist);
            bool traverseRight = Hit(right.bounds, r, invDir, rightDist);
            bool swap = !traverseLeft || (traverseRight && rightDist < leftDist);
            
            // push one of the nodes if both are intersected
            if (traverseLeft && traverseRight) {
                if (swap) { // push left node
                    nodesToVisit[toVisitOffset++] = left.nPrimitives;
                    nodesToVisit[toVisitOffset++] = left.firstChildOffset;
                }
                else { // push right node
                    nodesToVisit[toVisitOffset++] = right.nPrimitives;
                    nodesToVisit[toVisitOffset++] = right.firstChildOffset;
                }
            }
            // move to closest node
            if (traverseLeft || traverseRight) {
                if (swap) { // move to right node
                    currentNPrimitives = right.nPrimitives;
                    currentOffset = right.firstChildOffset;
                }
                else { // move to left node
                    currentNPrimitives = left.nPrimitives;
                    currentOffset = left.firstChildOffset;
                }
            }
            else {
                // pop next node to visit
                if (toVisitOffset == 0) break;
                currentOffset = nodesToVisit[--toVisitOffset];
                currentNPrimitives = nodesToVisit[--toVisitOffset];
            }
        }
    }
    return found;
}

std::shared_ptr<BVHAccel> BVHAccel::Create(const yocto::scene_shape &shape, BVHAccel::SplitMethod splitMethod) {
    clock_t start = clock();
    auto bvh = std::make_shared<BVHAccel>(shape, splitMethod);
    clock_t stop = clock();
    double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
    std::cerr << "BVH build time: " << timer_seconds << " seconds\n";

    return bvh;
}