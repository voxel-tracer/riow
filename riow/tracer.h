#pragma once

#include <vector>
#include "vec3.h"
#include "ray.h"
#include "tracer_callback.h"

class tracer {
public:
    virtual void Render(callback::callback_ptr cb = nullptr) = 0;
    virtual void RenderIteration(callback::callback_ptr cb = nullptr) = 0;
    virtual void DebugPixel(unsigned x, unsigned y, callback::callback_ptr cb) = 0;

    virtual unsigned numIterations() const = 0;
    virtual void updateCamera(double from_x, double from_y, double from_z,
        double at_x, double at_y, double at_z) = 0;
};