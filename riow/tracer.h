#pragma once

#include <vector>
#include "vec3.h"

class tracer {
public:
    virtual void Render() = 0;
    virtual void DebugPixel(unsigned x, unsigned y, std::vector<vec3>& paths) = 0;
};