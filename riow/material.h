#pragma once

#include "rtweekend.h"
#include "texture.h"

struct hit_record;

class material {
public:
    virtual bool scatter(const ray& in, const hit_record& rec, color& attenuation, ray& scattered) const = 0;
};

class lambertian : public material {
public:
    lambertian(const color& a) : albedo(make_shared<solid_color>(a)) {}
    lambertian(shared_ptr<texture> a) : albedo(a) {}

    virtual bool scatter(const ray& in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        auto scatter_direction = rec.normal + random_in_hemisphere(rec.normal);

        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray{ rec.p, scatter_direction };
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        return true;
    }

    shared_ptr<texture> albedo;
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

class dielectric: public material {
public:
    dielectric(double index_of_refraction) : ir(index_of_refraction) {}

    virtual bool scatter(const ray& in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        attenuation = color{ 1.0, 1.0, 1.0 };
        double refraction_ratio = rec.front_face ? (1.0 / ir) : ir;

        double cos_theta = fmin(dot(-in.direction(), rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ir) > random_double())
            direction = reflect(in.direction(), rec.normal);
        else
            direction = refract(in.direction(), rec.normal, refraction_ratio);

        scattered = ray{ rec.p, direction };
        return true;
    }

    double ir; // Index of Refraction

private:
    static double reflectance(double cosine, double ref_idx) {
        // Use Schlick's approximation for reflectance
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }
};
