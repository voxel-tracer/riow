#pragma once

#include <vector>
#include <yocto/yocto_math.h>

class tracer {
public:
    virtual void Render() = 0;
    virtual void DebugPixel(unsigned x, unsigned y, std::vector<yocto::vec3f>& paths) = 0;
};