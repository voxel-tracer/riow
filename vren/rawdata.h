#pragma once

#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstring>

#include "vec3.h"

class RawData {
protected:
    std::vector<vec3> data;
    unsigned width;
    unsigned height;

public:
    RawData(unsigned width, unsigned height) : width(width), height(height) {
        data.reserve(width * height);
    }

    RawData(std::string filename) {
        std::fstream in(filename, std::ios::in | std::ios::binary);

        const char* HEADER = "REF_00.01";
        int headerLen = strlen(HEADER) + 1;
        char* header = new char[headerLen];
        in.read(header, headerLen);
        if (strcmp(HEADER, header) != 0) {
            yocto::print_fatal("invalid header: " + std::string(header));
        }

        in.read((char*)&width, sizeof(unsigned));
        in.read((char*)&height, sizeof(unsigned));

        data.reserve(width * height);
        in.read((char*)&data[0], sizeof(vec3) * width * height);

        in.close();
    }

    void set(unsigned x, unsigned y, const vec3& v) {
        data[y * width + x] = v;
    }

    void saveToFile(std::string filename) const {
        std::fstream out(filename, std::ios::out | std::ios::binary);
        const char* HEADER = "REF_00.01";
        out.write(HEADER, strlen(HEADER) + 1);
        out.write((char*)&width, sizeof(unsigned));
        out.write((char*)&height, sizeof(unsigned));
        out.write((char*)&data[0], sizeof(vec3) * width * height);
        out.close();
    }

    double rmse(shared_ptr<const RawData> ref) const {
        if (ref->width != width || ref->height != height)
            throw std::invalid_argument("ref image has a different size");

        double error = 0.0;
        for (auto i = 0; i < width*height; i++) {
            const vec3 f = data[i];
            const vec3 g = ref->data[i];
            for (auto c = 0; c < 3; c++) {
                error += (f[c] - g[c]) * (f[c] - g[c]) / 3.0;
            }
        }
        return sqrt(error / (width * height));
    }

    bool findFirstDiff(shared_ptr<const RawData> ref, yocto::vec2i &coord) const {
        if (ref->width != width || ref->height != height)
            throw std::invalid_argument("ref image has a different size");
        for (auto i = 0; i < width * height; i++) {
            const vec3 f = data[i];
            const vec3 g = ref->data[i];
            if (f != g) {
                coord.x = i % width;
                coord.y = i / width;
                return true;
            }
        }

        return false;
    }
};