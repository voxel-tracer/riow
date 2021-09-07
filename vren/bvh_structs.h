#pragma once

#include <yocto/yocto_math.h>

struct Primitive {
    int element;
    yocto::bbox3f bounds;
};

// this is duplicated with bvh.cpp, but it's fine for now
struct LinearBVHNode {
    yocto::bbox3f bounds;
    union {
        int primitivesOffset;   // leaf
        int firstChildOffset;  // interior
    };
    uint16_t nPrimitives;   // 0 -> interior node
    uint8_t axis;           // interior node: xyz
    uint8_t pad[1];         // ensure 32 bytes total size
};

yocto::vec3f Diagonal(const yocto::bbox3f& b) { return b.max - b.min; }

float SurfaceArea(const yocto::bbox3f& b) {
    auto d = Diagonal(b);
    return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
}

int MaximumExtent(const yocto::bbox3f& b) {
    auto d = Diagonal(b);
    if (d.x > d.y && d.x > d.z)
        return 0;
    else if (d.y > d.z)
        return 1;
    else
        return 2;
}

yocto::vec3f Offset(const yocto::bbox3f& b, const yocto::vec3f& p) {
    auto o = p - b.min;
    for (auto a = 0; a < 3; a++)
        if (b.max[a] > b.min[a]) o[a] /= b.max[a] - b.min[a];
    return o;
}

yocto::bbox3f Intersect(const yocto::bbox3f& b1, const yocto::bbox3f& b2) {
    auto ret = b1;
    // _min must be in [node._min, node._max]
    ret.min = max(ret.min, b2.min);
    ret.min = min(ret.min, b2.max);
    // _max must be in [node._min, node._max]
    ret.max = max(ret.max, b2.min);
    ret.max = min(ret.max, b2.max);
    return ret;
}


bool Hit(const yocto::bbox3f& b, const yocto::ray3f& r, const yocto::vec3f& invDir, float &dist) {
    float t_min = r.tmin;
    float t_max = r.tmax;
    for (int a = 0; a < 3; a++) {
        float t0 = (b.min[a] - r.o[a]) * invDir[a];
        float t1 = (b.max[a] - r.o[a]) * invDir[a];
        if (invDir[a] < 0.0f) {
            float tmp = t0; t0 = t1; t1 = tmp;
        }
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max < t_min)
            return false;
    }

    dist = t_min;
    return true;
}
