#pragma once

#include <memory>
#include <yocto/yocto_image.h>
#include <yocto/yocto_sceneio.h>
#include <glm/vec3.hpp>
#include "Film.h"

class tracer;

namespace callback {
    class callback;
}

namespace tool {
    class window_base {
    public:
        virtual void render(int spp = -1) = 0;
        virtual void debugPixel(unsigned x, unsigned y, unsigned spp) = 0;

        virtual void showHisto(std::string title, std::vector<float> data) = 0;

        virtual void updateCamera(glm::vec3 look_from, glm::vec3 look_at) = 0;

        virtual void setCallback(callback::callback* cb) = 0;
    };

    std::shared_ptr<window_base> create_window(
        Film& film,
        std::shared_ptr<tracer> tr,
        std::shared_ptr<yocto::scene_model> sc,
        glm::vec3 look_at,
        glm::vec3 look_from);
}