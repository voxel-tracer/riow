#pragma once

#include <memory>

#include "hittable.h"
#include "ray.h"
#include "rnd.h"
#include "onb.h"

class plane : public hittable {
public:
    plane(std::string name, const point3& o, const vec3& n, std::shared_ptr<material> m) :
        hittable(name + "_plane"), origin(o), norm(unit_vector(n)), mat_ptr(m) {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, std::shared_ptr<rnd> rng) const override {
        double denom = dot(norm, r.direction());

        if (denom > -0.000001f) return false;
        vec3 po = origin - r.origin();
        float t = dot(po, norm) / denom;
        if (t < t_min || t > t_max) return false;

        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, norm);

        onb uv_onb = { norm };
        rec.u = dot(rec.p, uv_onb.u());
        rec.v = dot(rec.p, uv_onb.v());

        rec.mat_ptr = mat_ptr;

        return true;
    }

    const vec3 norm;
    const point3 origin;
    const std::shared_ptr<material> mat_ptr;
};