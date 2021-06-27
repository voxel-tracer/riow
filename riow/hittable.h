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

    virtual vec3 random(const point3& o, shared_ptr<rnd> rng) const {
        return vec3(1, 0, 0);
    }

    std::string name;
};

class flip_face : public hittable {
public:
    flip_face(shared_ptr<hittable> p) : hittable(p->name + "_flipped"), ptr(p) {}

    virtual bool hit(
        const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override {

        if (!ptr->hit(r, t_min, t_max, rec, rng))
            return false;

        rec.front_face = !rec.front_face;
        return true;
    }

public:
    shared_ptr<hittable> ptr;
};


class translate : public hittable {
public:
    translate(shared_ptr<hittable> p, const vec3& displacement)
        : hittable(p->name + "_translated"), ptr(p), offset(displacement) {}

    virtual bool hit(
        const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override {
        ray moved_r(r.origin() - offset, r.direction());
        if (!ptr->hit(moved_r, t_min, t_max, rec, rng))
            return false;

        rec.p += offset;
        rec.set_face_normal(moved_r, rec.normal);

        return true;
    }

public:
    shared_ptr<hittable> ptr;
    vec3 offset;
};

class rotate_y : public hittable {
public:
    rotate_y(shared_ptr<hittable> p, double angle): hittable(p->name + "_rotated_y"), ptr(p) {
        auto radians = degrees_to_radians(angle);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
    }

    virtual bool hit(
        const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override {
        auto origin = r.origin();
        auto direction = r.direction();

        origin[0] = cos_theta * r.origin()[0] - sin_theta * r.origin()[2];
        origin[2] = sin_theta * r.origin()[0] + cos_theta * r.origin()[2];

        direction[0] = cos_theta * r.direction()[0] - sin_theta * r.direction()[2];
        direction[2] = sin_theta * r.direction()[0] + cos_theta * r.direction()[2];

        ray rotated_r(origin, direction);

        if (!ptr->hit(rotated_r, t_min, t_max, rec, rng))
            return false;

        auto p = rec.p;
        auto normal = rec.normal;

        p[0] = cos_theta * rec.p[0] + sin_theta * rec.p[2];
        p[2] = -sin_theta * rec.p[0] + cos_theta * rec.p[2];

        normal[0] = cos_theta * rec.normal[0] + sin_theta * rec.normal[2];
        normal[2] = -sin_theta * rec.normal[0] + cos_theta * rec.normal[2];

        rec.p = p;
        rec.set_face_normal(rotated_r, normal);

        return true;
    }

public:
    shared_ptr<hittable> ptr;
    double sin_theta;
    double cos_theta;
};
