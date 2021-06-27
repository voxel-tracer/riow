#pragma once

#include "hittable.h"
#include "vec3.h"
#include "onb.h"

class sphere : public hittable {
public:
    sphere(std::string name): hittable(name) {}
    sphere(std::string name, point3 cen, double r, shared_ptr<material> m) :
        hittable(name), center(cen), radius(r), mat_ptr(m) {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const override;
    virtual double pdf_value(const point3& o, const vec3& v) const override;
    virtual vec3 random(const point3& o, shared_ptr<rnd> rng) const override;

private:
    static void get_sphere_uv(const point3& p, double& u, double& v) {
        // p: a given point on the sphere of radius one, centered at the origin.
        // u: returned value [0,1] of angle around the Y axis from X=-1.
        // v: returned value [0,1] of angle from Y=-1 to Y=+1.
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

        auto theta = std::acos(-p.y());
        auto phi = std::atan2(-p.z(), p.x()) + pi;

        u = phi / (2 * pi);
        v = theta / pi;
    }

public:
    point3 center;
    double radius;
    shared_ptr<material> mat_ptr;
};

bool sphere::hit(const ray& r, double t_min, double t_max, hit_record& rec, shared_ptr<rnd> rng) const {
    vec3 oc = r.origin() - center;
    auto half_b = dot(oc, r.direction());
    auto c = oc.length_squared() - radius * radius;

    auto discriminant = half_b * half_b - c;
    if (discriminant < 0) return false;
    auto sqrtd = sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range.
    auto root = (-half_b - sqrtd);
    if (root < t_min || t_max < root) {
        root = (-half_b + sqrtd);
        if (root < t_min || t_max < root)
            return false;
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    vec3 outward_normal = (rec.p - center) / radius;
    rec.set_face_normal(r, outward_normal);
    get_sphere_uv(outward_normal, rec.u, rec.v);
    rec.mat_ptr = mat_ptr;

    return true;
}

double sphere::pdf_value(const point3& o, const vec3& v) const {
    hit_record rec;
    if (!this->hit(ray(o, v), 0.001, infinity, rec, nullptr))
        return 0.0;

    auto cos_theta_max = sqrt(1 - radius * radius / (center - o).length_squared());
    auto solid_angle = 2 * pi * (1 - cos_theta_max);

    return  1 / solid_angle;
}

vec3 sphere::random(const point3& o, shared_ptr<rnd> rng) const {
    vec3 direction = center - o;
    auto distance_squared = direction.length_squared();
    onb uvw;
    uvw.build_from_w(direction);
    return uvw.local(rng->random_to_sphere(radius, distance_squared));
}