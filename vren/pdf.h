#pragma once

#include <yocto/yocto_sampling.h>

#include "rtweekend.h"
#include "onb.h"

class pdf {
public:
    virtual ~pdf() {}

    virtual double value(const vec3& direction) const = 0;
    virtual vec3 generate(shared_ptr<rnd> rng) = 0;
    virtual std::string name() const = 0;
};

class cosine_pdf : public pdf {
public:
    cosine_pdf(const vec3& w) : uvw(w) {}

    virtual double value(const vec3& direction) const {
        auto cosine = dot(unit_vector(direction), uvw.w());
        return cosine <= 0 ? 0 : cosine / pi;
    }

    virtual vec3 generate(shared_ptr<rnd> rng) {
        return uvw.local(rng->random_cosine_direction());
    }

    virtual std::string name() const override {
        return "cosine_pdf";
    }

    const onb uvw;
};

class hittable_pdf : public pdf {
public:
    hittable_pdf(shared_ptr<hittable> p, const point3& origin) : ptr(p), o(origin) {}

    virtual double value(const vec3& direction) const {
        return ptr->pdf_value(o, direction);
    }

    virtual vec3 generate(shared_ptr<rnd> rng) {
        return ptr->random(o, rng);
    }

    virtual std::string name() const override {
        return "hittable_pdf(name = " + ptr->pdf_name() + ")";
    }

    const point3 o;
    const shared_ptr<hittable> ptr;
};

class mixture_pdf : public pdf {
private:
    shared_ptr<pdf> chosen = nullptr;

public:
    mixture_pdf(shared_ptr<pdf> p0, shared_ptr<pdf> p1) {
        p[0] = p0;
        p[1] = p1;
    }

    virtual double value(const vec3& direction) const {
        return 0.5 * p[0]->value(direction) + 0.5 * p[1]->value(direction);
    }

    virtual vec3 generate(shared_ptr<rnd> rng) {
        chosen = (rng->random_double() < 0.5) ? p[0] : p[1];
        return chosen->generate(rng);
    }

    virtual std::string name() const override {
        return "mixture_pdf(" + chosen->name() + ")";
    }

    shared_ptr<pdf> p[2];
};

class uniform_sphere_pdf : public pdf {
public:
    uniform_sphere_pdf() { }

    virtual double value(const vec3& direction) const {
        return 1.0 / (4 * pi);
    }

    virtual vec3 generate(shared_ptr<rnd> rng) {
        return rng->random_in_unit_sphere();
    }

    virtual std::string name() const override {
        return "uniform_pdf";
    }
};
