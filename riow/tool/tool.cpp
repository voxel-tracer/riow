#include "tool.h"
#include "window.h"
#include "scene.h"

namespace tool {
    std::shared_ptr<window_base> create_window(
            std::shared_ptr<yocto::color_image> image, 
            std::shared_ptr<tracer> tr, 
            std::shared_ptr<yocto::scene_model> sc,
            glm::vec3 look_at, 
            glm::vec3 look_from) {
        auto w = make_shared<window>(image, tr, look_at, look_from);
        auto s = make_shared<scene>(*sc);
        w->set_scene(s);
        return w;
    }
}