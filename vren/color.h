#pragma once

#include <iostream>

#include "vec3.h"
#include <yocto/yocto_image.h>

//vec3 gamma_correct(color pixel_color, int samples_per_pixel) {
//    auto r = pixel_color.x();
//    auto g = pixel_color.y();
//    auto b = pixel_color.z();
//
//    // Replace NaN components with zero
//    if (std::isnan(r)) r = 0.0;
//    if (std::isnan(g)) g = 0.0;
//    if (std::isnan(b)) b = 0.0;
//
//    // Divide the color by the number of samples and gamma-correct for gamma=2.0
//    auto scale = 1.0 / samples_per_pixel;
//    r = sqrt(scale * r);
//    g = sqrt(scale * g);
//    b = sqrt(scale * b);
//
//    return vec3(
//        256 * clamp(r, 0.0, 0.999),
//        256 * clamp(g, 0.0, 0.999),
//        256 * clamp(b, 0.0, 0.999)
//    );
//}

//void write_color(std::ostream& out, color pixel_color, int samples_per_pixel) {
//    auto c = gamma_correct(pixel_color, samples_per_pixel);
//
//    // Write the translated [0, 255] value of each color component
//    out << static_cast<int>(c.x()) << ' ' << static_cast<int>(c.y()) << ' ' << static_cast<int>(c.z()) << '\n';
//}

inline yocto::vec4f convert(const color& pixel_color, unsigned spp, bool gamma_correct = true) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Replace NaN components with zero
    if (std::isnan(r)) r = 0.0;
    if (std::isnan(g)) g = 0.0;
    if (std::isnan(b)) b = 0.0;

    auto scale = 1.0 / spp;

    if (gamma_correct) {
        r = clamp(sqrt(scale * r), 0.0, 1.0);
        g = clamp(sqrt(scale * g), 0.0, 1.0);
        b = clamp(sqrt(scale * b), 0.0, 1.0);
    }
    else {
        r = scale * r;
        g = scale * g;
        b = scale * b;
    }

    return {
        static_cast<float>(r),
        static_cast<float>(g),
        static_cast<float>(b),
        1.0f
    };
}
