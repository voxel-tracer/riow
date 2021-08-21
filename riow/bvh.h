#pragma once

#include <memory>
#include <vector>

#include <yocto/yocto_scene.h>

#include "rtweekend.h"
#include "hit_record.h"

struct BVHBuildNode;
struct Primitive;
struct LinearBVHNode;
// BVHAccel Forward Declarations
struct BVHPrimitiveInfo;

class BVHAccel {
public:
    // BVHAccel Public Types
    enum class SplitMethod { SAH, EqualCounts };

    // BVHAccel Public Methods
    BVHAccel(const yocto::scene_shape& shape, SplitMethod splitMethod = SplitMethod::EqualCounts,
        int maxPrimsInNode = 1, float internalCost = 0.125f, bool reevaluateCost = false, bool useBestDim = true);
    ~BVHAccel();

    bool hit(yocto::ray3f r, yocto::vec2f& uv, int& element, float& dist) const;

    static std::shared_ptr<BVHAccel> Create(const yocto::scene_shape& shape, SplitMethod splitMethod = SplitMethod::EqualCounts);

private:
    // BVHAccel Private Methods
    BVHBuildNode* recursiveBuild(
        std::vector<BVHPrimitiveInfo>& primitiveInfo, 
        int start, int end, int* totalNodes, int *addedSplits);

    void flattenBVHTree(BVHBuildNode* node, int offset, int* firstChildOffset);
    void computeQuality(const BVHBuildNode* node, float rootSA, float* largestOverlap);
    float sahCost(const BVHBuildNode* node, float rootSA) const;

    // BVHAccel Private Data
    const int maxPrimsInNode;
    const SplitMethod splitMethod;
    const float internalCost;
    const bool reevaluateCost;
    const bool useBestDim;
    int convertedNodes = 0;
    int trimmedNodes = 0;
    float minOverlap;

    const yocto::scene_shape& shape;
    std::vector<int> elements;
    std::vector<LinearBVHNode> nodes;
};
