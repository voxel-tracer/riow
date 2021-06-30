#pragma once

#include "ray.h"
#include "hit_record.h"

class hittable {
public:
    hittable(std::string n) :name(n) {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const = 0;

    virtual double pdf_value(const point3& o, const vec3& v) const {
        return 0.0;
    }

    virtual vec3 random(const point3& o, shared_ptr<rnd> rng) {
        return vec3(1, 0, 0);
    }

    virtual std::string pdf_name() const {
        return "";
    }

    std::string name;
};