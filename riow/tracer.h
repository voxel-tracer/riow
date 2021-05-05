#pragma once

#include <vector>
#include "tool/tracer_data.h"

class tracer {
public:
    virtual void Render() = 0;
    virtual void DebugPixel(unsigned x, unsigned y, std::vector<tool::path_segment>& segments) = 0;
};