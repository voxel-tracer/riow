#pragma once

#include <iostream>

#include "vec3.h"

void write_color(std::ostream& out, color pixel_color, int samples_per_pixel) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Replace NaN components with zero
    if (std::isnan(r)) r = 0.0;
    if (std::isnan(g)) g = 0.0;
    if (std::isnan(b)) b = 0.0;

    // Divide the color by the number of samples and gamma-correct for gamma=2.0
    auto scale = 1.0 / samples_per_pixel;
    r = sqrt(scale * r);
    g = sqrt(scale * g);
    b = sqrt(scale * b);

    // Write the translated [0, 255] value of each color component
    out << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
}