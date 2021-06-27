#pragma once

#include <vector>
#include "vec3.h"
#include "ray.h"
#include "tracer_callback.h"

class tracer {
public:
    virtual void Render(callback::callback_ptr cb = nullptr) = 0;
    virtual void DebugPixel(unsigned x, unsigned y, callback::callback_ptr cb) = 0;
};