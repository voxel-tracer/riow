#pragma once

#include <yocto/yocto_math.h>

namespace tool {
    struct path_segment {
        yocto::vec3f s;
        yocto::vec3f e;
        yocto::vec3f c;
    };
}
