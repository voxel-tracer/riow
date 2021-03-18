#pragma once

#include "rtweekend.h"
#include "onb.h"

class pdf {
public:
    virtual ~pdf() {}

    virtual double value(const vec3& direction) const = 0;
    virtual vec3 generate() const = 0;
};

class cosine_pdf : public pdf {
public:
    cosine_pdf(const vec3& w) { uvw.build_from_w(w); }

    virtual double value(const vec3& direction) const {
        auto cosine = dot(unit_vector(direction), uvw.w());
        return cosine <= 0 ? 0 : cosine / pi;
    }

    virtual vec3 generate() const {
        return uvw.local(random_cosine_direction());
    }

public:
    onb uvw;
};