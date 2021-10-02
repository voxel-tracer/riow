#pragma once

#include <vector>
#include <yocto/yocto_math.h>
#include <yocto/yocto_image.h>
#include "rtweekend.h"
#include "color.h"
#include "rawdata.h"

class Film {
private:
    std::vector<yocto::vec4f> pixels;

public:
    const unsigned width;
    const unsigned height;

    Film(unsigned width, unsigned height) :
        width(width), height(height), pixels(width* height) {}

    void AddSample(int x, int y, const yocto::vec3f& c, double weight = 1.0) {
        pixels[y * width + x] += { c.x, c.y, c.z, (float)weight };
    }

    void GetImage(yocto::color_image& image) const {
        for (auto x = 0; x < width; x++) {
            for (auto y = 0; y < height; y++) {
                const auto& p = pixels[x + y * width];
                auto c = convert(vec3(p.x, p.y, p.z), (unsigned)p.w);
                yocto::set_pixel(image, x, y, c);
            }
        }
    }

    void GetRaw(RawData& raw) const {
        for (auto x = 0; x < width; x++) {
            for (auto y = 0; y < height; y++) {
                const auto& p = pixels[x + y * width];
                raw.set(x, y, vec3(p.x / p.w, p.y / p.w, p.z / p.w));
            }
        }
    }

    void Clear() {
        std::fill(pixels.begin(), pixels.end(), yocto::zero4f);
    }
};