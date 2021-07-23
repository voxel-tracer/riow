#pragma once

#include <vector>
#include "vec3.h"
#include "ray.h"
#include "tracer_callback.h"
#include "rawdata.h"

class tracer {
public:
    // TODO add a parallel flag instead of a separate method
    virtual void Render(unsigned spp, callback::callback_ptr cb = nullptr) = 0;
    virtual void RenderParallel(unsigned spp, callback::callback_ptr cb = nullptr) {}

    virtual void DebugPixel(unsigned x, unsigned y, unsigned spp, callback::callback_ptr cb) = 0;

    virtual unsigned numSamples() const = 0;
    virtual void updateCamera(double from_x, double from_y, double from_z,
        double at_x, double at_y, double at_z) = 0;
    // keep camera as is but resets rendering back to iteration 0
    virtual void Reset() = 0;

    virtual void getRawData(shared_ptr<RawData> data) const = 0;
};