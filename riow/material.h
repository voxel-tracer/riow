#pragma once

#include "rtweekend.h"

struct hit_record;

class material {
public:
    virtual bool scatter(const ray& in, const hit_record& rec, color& attenuation, ray& scattered) const = 0;
};

class lambertian : public material {
public:
    lambertian(const color& a) : albedo(a) {}

    virtual bool scatter(const ray& in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        auto scatter_direction = rec.normal + random_in_hemisphere(rec.normal);

        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray{ rec.p, scatter_direction };
        attenuation = albedo;
        return true;
    }

    color albedo;
};


class metal : public material {
public:
    metal(const color& a, double f = 0.0) : albedo(a), fuzz(f) {}

    virtual bool scatter(const ray& in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(in.direction(), rec.normal);
        scattered = ray{ rec.p, reflected + fuzz * random_in_unit_sphere() };
        attenuation = albedo;
        return (dot(reflected, rec.normal) > 0);
    }

    color albedo;
    double fuzz;
};