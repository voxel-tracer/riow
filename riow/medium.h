#pragma once

#include "rtweekend.h"
#include "rnd.h"

class Medium {
public:
    virtual double sampleDistance(shared_ptr<rnd> rng, double distance) const {
        return distance;
    }

    virtual vec3 sampleScatterDirection(shared_ptr<rnd> rng, const vec3& direction) const {
        return direction;
    }

    virtual color transmission(double distance) const = 0;
};

class NoScatterMedium : public Medium {
public:
    NoScatterMedium(color c, double d) : aColor(c), aDistance(d) {}

    virtual color transmission(double distance) const override {
        return pow(aColor, distance / aDistance);
    }

    color aColor; // absorption color
    double aDistance; // absorption at distance
};

class IsotropicScatteringMedium : public NoScatterMedium {
public:
    IsotropicScatteringMedium(color c, double ad, double sd) : NoScatterMedium(c, ad), scatteringDistance(sd) {}

    virtual double sampleDistance(shared_ptr<rnd> rng, double distance) const override {
        double d = -std::log(rng->random_double()) * scatteringDistance;
        if (d > distance) {
            return distance;
        }
        return d;
    }

    virtual vec3 sampleScatterDirection(shared_ptr<rnd> rng, const vec3& direction) const override {
        return rng->random_in_unit_sphere();
    }

    double scatteringDistance;
};