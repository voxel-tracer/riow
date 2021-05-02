#include "vec3.h"

std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v[0] << ' ' << v[1] << ' ' << v[2];
}

vec3 operator+(const vec3& u, const vec3& v) {
    return vec3{ u[0] + v[0], u[1] + v[1], u[2] + v[2] };
}

vec3 operator-(const vec3& u, const vec3& v) {
    return vec3{ u[0] - v[0], u[1] - v[1], u[2] - v[2] };
}

vec3 operator*(const vec3& u, const vec3& v) {
    return vec3{ u[0] * v[0], u[1] * v[1], u[2] * v[2] };
}

vec3 operator*(double t, const vec3& v) {
    return vec3{ v[0] * t, v[1] * t, v[2] * t };
}

vec3 operator*(const vec3& v, double t) {
    return t * v;
}

vec3 operator/(const vec3& v, double t) {
    return (1 / t) * v;
}

double dot(const vec3& u, const vec3& v) {
    return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
}

vec3 cross(const vec3& u, const vec3& v) {
    return vec3{
        u[1] * v[2] - u[2] * v[1],
        u[2] * v[0] - u[0] * v[2],
        u[0] * v[1] - u[1] * v[0]

    };
}

vec3 unit_vector(vec3 v) {
    return v / v.length();
}

vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2 * dot(v, n) * n;
}

vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    auto cos_theta = fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    double sqlen = r_out_perp.length_squared();
    vec3 r_out_parallel = sqlen >= 1.0f ? vec3(0, 0, 0) : -sqrt(1.0f - sqlen) * n;
    return r_out_perp + r_out_parallel;
}
