#pragma once

#include <cmath>

#include "vec3.h"

class rnd {
public:
    virtual double random_double() = 0;

    double random_double(double min, double max) {
        // Returns a random real in [min, max)
        return min + (max - min) * random_double();
    }

    int random_int(int min, int max) {
        // Returns a random integer in [min, max]
        return static_cast<int>(random_double(min, max + 1));
    }

    vec3 random_vec3() {
        return vec3{ random_double(), random_double(), random_double() };
    }

    vec3 random_vec3(double min, double max) {
        return vec3{ random_double(min, max), random_double(min, max), random_double(min, max) };
    }

    vec3 random_in_unit_sphere() {
        while (true) {
            auto p = random_vec3(-1, 1);
            if (p.length_squared() >= 1) continue;
            return p;
        }
    }

    vec3 random_unit_vector() {
        return unit_vector(random_in_unit_sphere());
    }

    vec3 random_in_hemisphere(const vec3& normal) {
        vec3 in_unit_sphere = random_in_unit_sphere();
        if (dot(in_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
            return in_unit_sphere;
        else
            return -in_unit_sphere;
    }

    vec3 random_in_unit_disk() {
        while (true) {
            auto p = vec3{ random_double(-1,1), random_double(-1,1), 0 };
            if (p.length_squared() >= 1.0) continue;
            return p;
        }
    }

    vec3 random_cosine_direction() {
        auto r1 = random_double();
        auto r2 = random_double();
        auto z = sqrt(1 - r2);

        auto phi = 2 * pi * r1;
        auto x = cos(phi) * sqrt(r2);
        auto y = sin(phi) * sqrt(r2);

        return vec3(x, y, z);
    }
    
    vec3 random_to_sphere(double radius, double distance_squared) {
        auto r1 = random_double();
        auto r2 = random_double();
        auto z = 1 + r2 * (sqrt(1 - radius * radius / distance_squared) - 1);

        auto phi = 2 * pi * r1;
        auto x = cos(phi) * sqrt(1 - z * z);
        auto y = sin(phi) * sqrt(1 - z * z);

        return vec3(x, y, z);
    }
};

class default_rnd : public rnd {
public:
    virtual double random_double() override {
        // Returns a random real in [0, 1)
        return rand() / (RAND_MAX + 1.0);
    }
};