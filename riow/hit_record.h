#pragma once

#include <memory>
#include "vec3.h"

class material;
class hittable;

struct hit_record {
    point3 p;
    vec3 normal;
    std::shared_ptr<material> mat_ptr;
    double t;
    double u;
    double v;
    bool front_face;
    std::shared_ptr<hittable> obj_ptr;

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }

};
