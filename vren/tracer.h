#pragma once

#include <vector>
#include "vec3.h"
#include "ray.h"
#include "tracer_callback.h"
#include "rawdata.h"

class tracer {
public:
    virtual void Render(unsigned spp, bool parallel = true, callback::callback* cb = nullptr) {}

    virtual void DebugPixel(unsigned x, unsigned y, unsigned spp, callback::callback* cb) = 0;

    virtual void updateCamera(double from_x, double from_y, double from_z,
        double at_x, double at_y, double at_z) = 0;
    // keep camera as is but resets rendering back to iteration 0
    virtual void Reset() = 0;
};