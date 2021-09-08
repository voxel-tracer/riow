#pragma once

#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_sampling.h>

#include "vec3.h"
#include "pdf.h"

#include <iostream>

yocto::vec2i dir2ij(const vec3 direction, int texture_width, int texture_height) {
    // placeholder to later transform direction
    const auto wl = yocto::vec3f{ (float)direction[0], (float)direction[1], (float)direction[2] };

    auto texcoord = yocto::vec2f{
        yocto::atan2(wl.z, wl.x) / (2 * yocto::pif),
        yocto::acos(yocto::clamp(wl.y,-1.0f,1.0f)) / yocto::pif
    };
    if (texcoord.x < 0) texcoord.x += 1;

    return yocto::vec2i{
        yocto::clamp((int)(texcoord.x * texture_width), 0, texture_width - 1),
        yocto::clamp((int)(texcoord.y * texture_height), 0, texture_height - 1)
    };
}

class EnvMapPdf : public pdf {
private:
    const yocto::color_image &texture;
    const std::vector<float> elements_cdf;

    static std::vector<float> computeElementsCdf(const yocto::color_image& texture) {
        auto cdf = std::vector<float>(texture.width * texture.height);
        for (auto idx = 0; idx < cdf.size(); idx++) {
            auto ij = yocto::vec2i{ idx % texture.width, idx / texture.width };
            auto th = (ij.y + 0.5f) * yocto::pif / texture.height;
            auto value = yocto::get_pixel(texture, ij.x, ij.y);
            // multiply pixel value by sin(th) to account for the row stretch near the pole
            cdf[idx] = yocto::max(value) * yocto::sin(th);
            if (idx != 0) cdf[idx] += cdf[idx - 1];
        }
        return cdf;
    }

public:
    EnvMapPdf(const yocto::color_image& tex) : 
        texture(tex), elements_cdf(EnvMapPdf::computeElementsCdf(tex)) {}

    virtual double value(const vec3& direction) const override {
        auto ij = dir2ij(direction, texture.width, texture.height);
        auto prob = yocto::sample_discrete_pdf(elements_cdf, ij.y * texture.width + ij.x) / elements_cdf.back();
        auto angle = (2 * yocto::pif / texture.width) * (yocto::pif / texture.height) * 
            yocto::sin(yocto::pif * (ij.y + 0.5f) / texture.height);
        return prob / angle;
    }

    virtual vec3 generate(rnd& rng) override {
        // pick a random pixel from the env map according to the CDF
        auto idx = yocto::sample_discrete(elements_cdf, (float)rng.random_double());
        // compute normalized uv coordinates of the pixel's center
        auto uv = yocto::vec2f{
            ((idx % texture.width) + 0.5f) / texture.width,
            ((idx / texture.width) + 0.5f) / texture.height
        };
        auto sample = yocto::vec3f{
            yocto::cos(uv.x * 2 * yocto::pif) * yocto::sin(uv.y * yocto::pif),
            yocto::cos(uv.y * yocto::pif),
            yocto::sin(uv.x * 2 * yocto::pif) * yocto::sin(uv.y * yocto::pif)
        };

        return vec3(sample.x, sample.y, sample.z);
    }

    virtual std::string name() const override {
        return "envmap_pdf";
    }
};

class EnvMap {
private:
    const yocto::color_image texture;

public:
    EnvMap(std::string filename) :
        texture(EnvMap::loadImage(filename)), pdf(std::make_shared<EnvMapPdf>(texture)) {}

    color value(const vec3& direction) const {
        auto ij = dir2ij(direction, texture.width, texture.height);
        auto v = yocto::get_pixel(texture, ij.x, ij.y);
        return color(v.x, v.y, v.z);
    }

    static yocto::color_image loadImage(std::string filename) {
        auto error = std::string{};
        auto image = yocto::color_image{};
        if (!yocto::load_image(filename, image, error))
            yocto::print_fatal("Failed to load image: " + error);

        return image;
    }

    const std::shared_ptr<EnvMapPdf> pdf;
};