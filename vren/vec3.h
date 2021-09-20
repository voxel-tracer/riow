#pragma once

#include <cmath>
#include <ostream>

#include <yocto/yocto_geometry.h>

class vec3 {
public:
    vec3() :e{ 0, 0, 0 } { }
    vec3(double a) : e{ a, a, a } {}
    vec3(int a) {
        e[0] = ((a >> 16) & 0xFF) / 255.0;
        e[1] = ((a >> 8) & 0xFF) / 255.0;
        e[2] = ((a) & 0xFF) / 255.0;
    }
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

    vec3& operator*=(const vec3& v) {
        e[0] *= v[0];
        e[1] *= v[1];
        e[2] *= v[2];
        return *this;
    }

    vec3& operator/=(const double t) {
        e[0] /= t;
        e[1] /= t;
        e[2] /= t;
        return *this;
    }

    double length() const {
        return std::sqrt(length_squared());
    }

    double length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    bool near_zero() const {
        // Return true if the vector is close to zero in all dimensions
        const auto s = 1e-8;
        return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
    }

public:
    double e[3];
};

// Type aliases for vec3
using point3 = vec3; // 3D point
using color = vec3; // RGB color

// vec3 Utility functions
inline bool operator!=(const vec3& u, const vec3& v) {
    return u[0] != v[0] || u[1] != v[1] || u[2] != v[2];
}

inline bool operator==(const vec3& u, const vec3& v) {
    return u[0] == v[0] && u[1] == v[1] && u[2] == v[2];
}

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

inline double max(const vec3& v) {
    return std::max(v.x(), std::max(v.y(), v.z()));
}

inline vec3 exp(const vec3& v) {
    return { std::exp(v[0]), std::exp(v[1]), std::exp(v[2]) };
}

inline vec3 pow(const vec3& v, float a) {
    return { std::pow(v[0], a), std::pow(v[1], a), std::pow(v[2], a) };
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

inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2 * dot(v, n) * n;
}

inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    auto cos_theta = fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    double sqlen = r_out_perp.length_squared();
    vec3 r_out_parallel = sqlen >= 1.0f ? vec3(0, 0, 0) : -std::sqrt(1.0f - sqlen) * n;
    return r_out_perp + r_out_parallel;
}

inline yocto::vec3f toYocto(const vec3& v) {
    return { (float)v[0], (float)v[1], (float)v[2] };
}

inline vec3 fromYocto(const yocto::vec3f& v) {
    return vec3(v.x, v.y, v.z);
}