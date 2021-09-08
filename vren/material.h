#pragma once

#include "rtweekend.h"
#include "medium.h"
#include "texture.h"
#include "onb.h"
#include "pdf.h"

struct hit_record;

struct scatter_record {
    ray specular_ray;
    bool is_specular = false;
    bool is_refracted = false;
    color attenuation = { 1.0, 1.0, 1.0 };
    std::shared_ptr<pdf> pdf_ptr{}; // it's ok for this to be shared as it will never get copied
    Medium* medium_ptr = nullptr;
};

class material {
public:
    virtual color emitted(const ray& in, const hit_record& rec, double u, double v, const point3& p) const {
        return color(0, 0, 0);
    }

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, rnd& rng) const {
        return false;
    }

    virtual double scattering_pdf(const ray& in, const hit_record& rec, const ray& scattered) const {
        return 0;
    }
};

/*
 incoming ray scatters through the material in the same direction
 incoming ray attenuation changes according to which side of the surface it hit:
 . 20% of red channel for front hits
 . 20% of blue channel for back hits
*/
class passthrough : public material {
private:
    std::shared_ptr<Medium> medium = std::make_shared<InvisibleMedium>();

public:
    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, rnd& rng) const override {
        srec.specular_ray = { rec.p, in.direction() };
        srec.is_specular = true;
        srec.is_refracted = true;
        srec.medium_ptr = medium.get();
        srec.attenuation = rec.front_face ? color(.5, 1.0, 1.0) : color(1.0, 1.0, .5);
        return true;
    }
};

class lambertian : public material {
public:
    lambertian(const color& a) : albedo(make_shared<solid_color>(a)) {}
    lambertian(shared_ptr<texture> a) : albedo(a) {}

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, rnd& rng) const override {
        srec.is_specular = false;
        srec.attenuation = albedo->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = std::make_shared<cosine_pdf>(rec.normal);
        return true;
    }

    double scattering_pdf(const ray& in, const hit_record& rec, const ray& scattered) const override {
        auto cosine = dot(rec.normal, scattered.direction());
        return cosine < 0.0 ? 0.0 : cosine / pi;
    }

    shared_ptr<texture> albedo;
};

class metal : public material {
public:
    metal(const color& a, double f = 0.0) : albedo(a), fuzz(f) {}

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, rnd& rng) const override {
        vec3 reflected = reflect(in.direction(), rec.normal);
        srec.specular_ray = ray{ rec.p, reflected + fuzz * rng.random_in_unit_sphere() };
        srec.attenuation = albedo;
        srec.is_specular = true;
        srec.pdf_ptr = nullptr;
        return (dot(reflected, rec.normal) > 0);
    }

    color albedo;
    double fuzz;
};

class dielectric: public material {
public:
    dielectric(double index_of_refraction, shared_ptr<Medium> m = {}) : ir(index_of_refraction) {
        medium = m ? m : std::make_shared<InvisibleMedium>();
    }

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, rnd& rng) const override {
        srec.is_specular = true;
        srec.pdf_ptr = nullptr;
        srec.attenuation = color{ 1.0, 1.0, 1.0 };
        srec.medium_ptr = medium.get();
        double refraction_ratio = rec.front_face ? (1.0 / ir) : ir;

        double cos_theta = fmin(dot(-in.direction(), rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ir) > rng.random_double()) {
            direction = reflect(in.direction(), rec.normal);
            srec.is_refracted = false;
        } else {
            direction = refract(in.direction(), rec.normal, refraction_ratio);
            srec.is_refracted = true;
        }

        srec.specular_ray = ray{ rec.p, direction };
        return true;
    }

    double ir; // Index of Refraction
    std::shared_ptr<Medium> medium;

private:
    static double reflectance(double cosine, double ref_idx) {
        // Use Schlick's approximation for reflectance
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }
};

class diffuse_light : public material {
public:
    diffuse_light(shared_ptr<texture> a) : emit(a) {}
    diffuse_light(color c) : emit(make_shared<solid_color>(c)) {}

    virtual color emitted(const ray& in, const hit_record& rec, double u, double v, const point3& p) const override {
        if (rec.front_face)
            return emit->value(u, v, p);
        return color(0, 0, 0);
    }

public:
    shared_ptr<texture> emit;
};

class diffuse_subsurface_scattering : public material {
public:
    diffuse_subsurface_scattering(color a, shared_ptr<Medium> m) : albedo(a), medium(m) {}

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, rnd& rng) const override {
        if (rec.front_face) {
            // ray hit surface of the material from the outside
            // scatter ray on a sphere
            srec.specular_ray = ray(rec.p, rng.random_in_unit_sphere());
            srec.is_specular = true;
            if (dot(rec.normal, srec.specular_ray.direction()) < 0) {
                // ray entered the medium
                srec.is_refracted = true;
                srec.medium_ptr = medium.get();
            }
            else {
                // ray scattered at the surface
                srec.is_refracted = false;
                srec.medium_ptr = nullptr;
                srec.attenuation = albedo;
            }
        } else {
            // ray is exiting the medium
            srec.is_specular = true;
            srec.is_refracted = true;
            srec.specular_ray = ray(rec.p, in.direction());
        }

        return true;
    }

    color albedo;
    shared_ptr<Medium> medium;
};