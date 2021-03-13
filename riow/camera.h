#pragma once

#include "rtweekend.h"

class camera {
public:
    camera(
        point3 look_from,
        point3 look_at,
        vec3 vup,
        double vfov, // vertical field-of-view in degrees
        double aspect_ratio) {
        auto theta = degrees_to_radians(vfov);
        auto h = tan(theta / 2);
        auto viewport_height = 2.0 * h;
        auto viewport_width = aspect_ratio * viewport_height;

        auto w = unit_vector(look_from - look_at);
        auto u = unit_vector(cross(vup, w));
        auto v = cross(w, u);

        origin = look_from;
        horizontal = viewport_width * u;
        vertical = viewport_height * v;
        lower_left_corner = origin - horizontal / 2 - vertical / 2 - w;
    }

    ray get_ray(double s, double t) const {
        return ray{ origin, lower_left_corner + s * horizontal + t * vertical - origin };
    }

private:
    point3 origin;
    point3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
};