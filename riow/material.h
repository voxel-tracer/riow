#pragma once

#include "rtweekend.h"
#include "medium.h";
#include "texture.h"
#include "onb.h"
#include "pdf.h"

struct hit_record;

struct scatter_record {
    ray specular_ray;
    bool is_specular = false;
    bool is_refracted = false;
    color attenuation = { 1.0, 1.0, 1.0 };
    shared_ptr<pdf> pdf_ptr = nullptr;
    shared_ptr<Medium> medium_ptr = nullptr;
};

class material {
public:
    virtual color emitted(const ray& in, const hit_record& rec, double u, double v, const point3& p) const {
        return color(0, 0, 0);
    }

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, shared_ptr<rnd> rng) const {
        return false;
    }

    virtual double scattering_pdf(const ray& in, const hit_record& rec, const ray& scattered) const {
        return 0;
    }
};

class lambertian : public material {
public:
    lambertian(const color& a) : albedo(make_shared<solid_color>(a)) {}
    lambertian(shared_ptr<texture> a) : albedo(a) {}

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, shared_ptr<rnd> rng) const override {
        srec.is_specular = false;
        srec.attenuation = albedo->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = make_shared<cosine_pdf>(rec.normal);
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

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, shared_ptr<rnd> rng) const override {
        vec3 reflected = reflect(in.direction(), rec.normal);
        srec.specular_ray = ray{ rec.p, reflected + fuzz * rng->random_in_unit_sphere() };
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
    dielectric(double index_of_refraction, shared_ptr<Medium> m = {}) : ir(index_of_refraction), medium(m) {}

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, shared_ptr<rnd> rng) const override {
        srec.is_specular = true;
        srec.pdf_ptr = nullptr;
        srec.attenuation = color{ 1.0, 1.0, 1.0 };
        srec.medium_ptr = medium;
        double refraction_ratio = rec.front_face ? (1.0 / ir) : ir;

        double cos_theta = fmin(dot(-in.direction(), rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ir) > rng->random_double()) {
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

class isotropic : public material {
public:
    isotropic(color c) : albedo(make_shared<solid_color>(c)) {}
    isotropic(shared_ptr<texture> a) : albedo(a) {}

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, shared_ptr<rnd> rng) const override {
        srec.is_specular = false;
        srec.attenuation = albedo->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = make_shared<uniform_sphere_pdf>();
        return true;
    }

    double scattering_pdf(const ray& in, const hit_record& rec, const ray& scattered) const override {
        return 1 / (4 * pi);
    }

public:
    shared_ptr<texture> albedo;
};

class diffuse_subsurface_scattering : public lambertian {
public:
    diffuse_subsurface_scattering(double w, const color& c, shared_ptr<Medium> m) :
        lambertian(c), weight(w), medium(m) {}

    virtual bool scatter(const ray& in, const hit_record& rec, scatter_record& srec, shared_ptr<rnd> rng) const override {
        if (rec.front_face) {
            // ray coming from outside the model
            if (rng->random_double() < weight) {
                // ray is entering the medium
                srec.is_specular = true;
                srec.is_refracted = true;
                srec.medium_ptr = medium;
                srec.specular_ray = ray(rec.p, in.direction());
                return true;
            } else {
                // lambertian scattering
                return lambertian::scatter(in, rec, srec, rng);
            }
        } else {
            // ray is exiting the medium
            srec.is_specular = true;
            srec.is_refracted = true;
            srec.specular_ray = ray(rec.p, in.direction());
            return true;
        }
    }

    double weight; // how much of the scattered rays are lambertian
    shared_ptr<Medium> medium;
};