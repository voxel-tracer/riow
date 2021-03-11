#pragma once

#include <cmath>
#include <iostream>

class vec3 {
public:
    vec3() :e{ 0, 0, 0 } { }
    vec3(double e0, double e1, double e2) :e{ e0, e1, e2 } { }

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3& v) {
        e[0] += v[0];
        e[1] += v[1];
        e[2] += v[2];
        return *this;
    }

    vec3& operator*=(const double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(const double t) {
        e[0] /= t;
        e[1] /= t;
        e[2] /= t;
        return *this;
    }

    double length() const {
        return sqrt(length_squared());
    }

    double length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    inline static vec3 random() {
        return vec3{ random_double(), random_double(), random_double() };
    }

    inline static vec3 random(double min, double max) {
        return vec3{ random_double(min, max), random_double(min, max), random_double(min, max) };
    }

public:
    double e[3];
};

// Type aliases for vec3
using point3 = vec3; // 3D point
using color = vec3; // RGB color

// vec3 Utility functions

inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v[0] << ' ' << v[1] << ' ' << v[2];
}

inline vec3 operator+(const vec3& u, const vec3& v) {
    return vec3{ u[0] + v[0], u[1] + v[1], u[2] + v[2] };
}

inline vec3 operator-(const vec3& u, const vec3& v) {
    return vec3{ u[0] - v[0], u[1] - v[1], u[2] - v[2] };
}

inline vec3 operator*(const vec3& u, const vec3& v) {
    return vec3{ u[0] * v[0], u[1] * v[1], u[2] * v[2] };
}

inline vec3 operator*(double t, const vec3& v) {
    return vec3{ v[0] * t, v[1] * t, v[2] * t };
}

inline vec3 operator*(const vec3& v, double t) {
    return t * v;
}

inline vec3 operator/(const vec3& v, double t) {
    return (1 / t) * v;
}

inline double dot(const vec3& u, const vec3& v) {
    return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
}

inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3{
        u[1] * v[2] - u[2] * v[1],
        u[2] * v[0] - u[0] * v[2],
        u[0] * v[1] - u[1] * v[0]

    };
}

inline vec3 unit_vector(vec3 v) {
    return v / v.length();
}

vec3 random_in_unit_sphere() {
    while (true) {
        auto p = vec3::random(-1, 1);
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