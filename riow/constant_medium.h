#pragma once

#include "rtweekend.h"

#include "hittable.h"
#include "material.h"
#include "texture.h"


class constant_medium : public hittable {
public:
    constant_medium(shared_ptr<hittable> b, double d, shared_ptr<texture> a)
        : boundary(b), neg_inv_density(-1 / d), phase_function(make_shared<isotropic>(a)) {}
    constant_medium(shared_ptr<hittable> b, double d, color c)
        : boundary(b), neg_inv_density(-1 / d), phase_function(make_shared<isotropic>(c)) {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override;

public:
    shared_ptr<hittable> boundary;
    shared_ptr<material> phase_function;
    double neg_inv_density;

};


bool constant_medium::hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const {
    hit_record rec1, rec2;

    if (!boundary->hit(r, -infinity, infinity, rec1, rng))
        return false;

    if (!boundary->hit(r, rec1.t + 0.0001, infinity, rec2, rng))
        return false;

    if (rec1.t < t_min) rec1.t = t_min;
    if (rec2.t > t_max) rec2.t = t_max;

    if (rec1.t >= rec2.t)
        return false;

    if (rec1.t < 0)
        rec1.t = 0;

    const auto ray_length = r.direction().length(); // always 1 because r.direction is normalized
    const auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
    const auto hit_distance = neg_inv_density * log(rng->random_double());

    if (hit_distance > distance_inside_boundary)
        return false;

    rec.t = rec1.t + hit_distance / ray_length;
    rec.p = r.at(rec.t);

    rec.normal = vec3(1, 0, 0);  // arbitrary
    rec.front_face = true;     // also arbitrary
    rec.mat_ptr = phase_function;

    return true;
}
