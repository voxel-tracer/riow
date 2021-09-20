#pragma once

#include "rtweekend.h"
#include "rnd.h"

class Medium {
public:
    // Sample scattering distance
    // Returns beam transmittance
    virtual vec3 SampleDistance(const double tMax, double& distance, rnd& rng) const {
        distance = tMax;
        return { 1.0, 1.0, 1.0 };
    }

    virtual void SampleDirection(const vec3 &wo, vec3 &wi, rnd&rng) const {
        wi = wo;
    }
};

class NoScatterMedium : public Medium {
public:
    NoScatterMedium(color c, double d) : aColor(c), aDistance(d) {}

    virtual vec3 SampleDistance(const double tMax, double& distance, rnd& rng) const override {
        distance = tMax;
        return pow(aColor, distance / aDistance);
    }

    const color aColor; // absorption color
    const double aDistance; // absorption at distance
};

class HomogeneousMedium : public Medium {
private:
    const vec3 sigma_a;
    const vec3 sigma_s;
    const vec3 sigma_t;

public:
    HomogeneousMedium(const vec3& sigma_a, const vec3& sigma_s) :
        sigma_a(sigma_a), sigma_s(sigma_s), sigma_t(sigma_a + sigma_s) {}

    virtual vec3 SampleDistance(const double tMax, double& distance, rnd& rng) const {
        // Sample a channel and distance along the ray
        auto channel = std::min((int)(rng.random_double() * 3), 2);
        double dist = -std::log(1 - rng.random_double()) / sigma_t[channel];
        distance = std::min(dist, tMax);
        bool sampledMedium = distance < tMax;

        // Compute the transmittance and sampling density
        auto Tr = exp(-sigma_t * distance);

        // Return weighting factor for scattering from homogeneous medium
        auto density = sampledMedium ? (sigma_t * Tr) : Tr;
        double pdf = 0;
        for (int i = 0; i < 3; ++i) pdf += density[i];
        pdf *= 1 / 3.0;
        if (pdf == 0) {
            pdf = 1;
        }
        return sampledMedium ? (Tr * sigma_s / pdf) : (Tr / pdf);
    }

    virtual void SampleDirection(const vec3& wo, vec3& wi, rnd& rng) const {
        wi = rng.random_in_unit_sphere();
    }
};