#pragma once

#include <iostream>

#include "vec3.h"

void write_color(std::ostream& out, color pixel_color) {
    // Write the translated [0, 255] value of each color component
    out << static_cast<int>(255.99 * pixel_color[0]) << ' '
        << static_cast<int>(255.99 * pixel_color[1]) << ' '
        << static_cast<int>(255.99 * pixel_color[2]) << '\n';
}