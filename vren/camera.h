#pragma once

#include "rtweekend.h"

class camera {
public:
    camera(
        point3 lookfrom,
        point3 lookat,
        vec3 vup,
        double vfov, // vertical field-of-view in degrees
        double aspect_ratio,
        double aperture): vup(vup) {
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        viewport_height = 2.0 * h;
        viewport_width = aspect_ratio * viewport_height;

        update(lookfrom, lookat);

        lens_radius = aperture / 2;
    }

    ray get_ray(double s, double t, shared_ptr<rnd> rng) const {
        vec3 rd = lens_radius * rng->random_in_unit_disk();
        vec3 offset = u * rd.x() + v * rd.y();

        return ray{
            lookfrom + offset,
            lower_left_corner + s * horizontal + t * vertical - lookfrom - offset };
    }

    point3 getLookAt() const { return lookat; }
    point3 getLookFrom() const { return lookfrom; }

    void update(point3 from, point3 at) {
        lookfrom = from;
        lookat = at;

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        auto focus_dist = (lookfrom - lookat).length();
        horizontal = focus_dist * viewport_width * u;
        vertical = focus_dist * viewport_height * v;
        lower_left_corner = lookfrom - horizontal / 2 - vertical / 2 - focus_dist * w;
    }

private:
    const vec3 vup;
    double viewport_width;
    double viewport_height;

    point3 lookat;
    point3 lookfrom;
    point3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    double lens_radius;
};