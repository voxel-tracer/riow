#pragma once

#include "rtweekend.h"

#include "hittable.h"


class xy_rect : public hittable {
public:
    xy_rect() {}

    xy_rect(
        double _x0, double _x1, double _y0, double _y1, double _k, shared_ptr<material> mat
    ) : x0(_x0), x1(_x1), y0(_y0), y1(_y1), k(_k), mp(mat) {};

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override;

public:
    shared_ptr<material> mp;
    double x0, x1, y0, y1, k;
};

class xz_rect : public hittable {
public:
    xz_rect() {}

    xz_rect(
        double _x0, double _x1, double _z0, double _z1, double _k, shared_ptr<material> mat
    ) : x0(_x0), x1(_x1), z0(_z0), z1(_z1), k(_k), mp(mat) {};

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override;

    virtual double pdf_value(const point3& origin, const vec3& v) const override {
        hit_record rec;
        if (!this->hit(ray(origin, v), 0.001, infinity, rec, nullptr))
            return 0.0;

        auto area = (x1 - x0) * (z1 - z0);
        auto distance_squared = rec.t * rec.t * v.length_squared();
        auto cosine = fabs(dot(v, rec.normal) / v.length());

        return distance_squared / (cosine * area);
    }

    virtual vec3 random(const point3& origin, shared_ptr<rnd> rng) const override {
        auto random_point = point3(rng->random_double(x0, x1), k, rng->random_double(z0, z1));
        return random_point - origin;
    }

public:
    shared_ptr<material> mp;
    double x0, x1, z0, z1, k;
};

class yz_rect : public hittable {
public:
    yz_rect() {}

    yz_rect(
        double _y0, double _y1, double _z0, double _z1, double _k, shared_ptr<material> mat
    ) : y0(_y0), y1(_y1), z0(_z0), z1(_z1), k(_k), mp(mat) {};

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override;

public:
    shared_ptr<material> mp;
    double y0, y1, z0, z1, k;
};

bool xy_rect::hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const {
    auto t = (k - r.origin().z()) / r.direction().z();
    if (t < t_min || t > t_max)
        return false;

    auto x = r.origin().x() + t * r.direction().x();
    auto y = r.origin().y() + t * r.direction().y();
    if (x < x0 || x > x1 || y < y0 || y > y1)
        return false;

    rec.u = (x - x0) / (x1 - x0);
    rec.v = (y - y0) / (y1 - y0);
    rec.t = t;
    auto outward_normal = vec3(0, 0, 1);
    rec.set_face_normal(r, outward_normal);
    rec.mat_ptr = mp;
    rec.p = r.at(t);

    return true;
}

bool xz_rect::hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const {
    auto t = (k - r.origin().y()) / r.direction().y();
    if (t < t_min || t > t_max)
        return false;

    auto x = r.origin().x() + t * r.direction().x();
    auto z = r.origin().z() + t * r.direction().z();
    if (x < x0 || x > x1 || z < z0 || z > z1)
        return false;

    rec.u = (x - x0) / (x1 - x0);
    rec.v = (z - z0) / (z1 - z0);
    rec.t = t;
    auto outward_normal = vec3(0, 1, 0);
    rec.set_face_normal(r, outward_normal);
    rec.mat_ptr = mp;
    rec.p = r.at(t);

    return true;
}

bool yz_rect::hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const {
    auto t = (k - r.origin().x()) / r.direction().x();
    if (t < t_min || t > t_max)
        return false;

    auto y = r.origin().y() + t * r.direction().y();
    auto z = r.origin().z() + t * r.direction().z();
    if (y < y0 || y > y1 || z < z0 || z > z1)
        return false;

    rec.u = (y - y0) / (y1 - y0);
    rec.v = (z - z0) / (z1 - z0);
    rec.t = t;
    auto outward_normal = vec3(1, 0, 0);
    rec.set_face_normal(r, outward_normal);
    rec.mat_ptr = mp;
    rec.p = r.at(t);

    return true;
}