#pragma once

#include "tool.h"

#include "imguiManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <yocto/yocto_image.h>

#include "camera.h"
#include "shader.h"
#include "scene.h"
#include "screen_texture.h"
#include "zoom_pixel.h"
#include "lines.h"
#include "../tracer.h"

namespace tool {
    enum WindowState {
        PathTracer,
        WireFrame,
        HitView
    };

    class window : public window_base {
    private:
        WindowState state = WindowState::PathTracer;
        bool esc_pressed = false;
        double mouse_last_x = 0.0f;
        double mouse_last_y = 0.0f;

        camera cam;
        GLFWwindow* glwindow;

        unique_ptr<Shader> shader;
        unique_ptr<ImGuiManager> imGuiManager;
        
        // needed by WindowState::ViewHits
        std::vector<std::shared_ptr<callback::Hit>> hits{};
        unsigned currentHitIdx = 0;
        bool n_pressed = false;

        std::shared_ptr<scene> scene;

        unique_ptr<screen_texture> screen;
        unique_ptr<zoom_pixel> pixel;
        unique_ptr<lines> ls = NULL;

        shared_ptr<tracer> pt;
        shared_ptr<callback::callback> cb;
        bool isRendering = false;

        void switchToWireFrame(bool force);
        void switchToPathTracer(bool force);

    public:
        window(std::shared_ptr<yocto::color_image> image, shared_ptr<tracer> tr, glm::vec3 look_at, glm::vec3 look_from);
        ~window();

        void set_scene(shared_ptr<tool::scene> sc) { scene = sc; }

        virtual void setCallback(shared_ptr<callback::callback> c) override { cb = c; }

        virtual void render(int spp = -1) override;

        virtual void debugPixel(unsigned x, unsigned y, unsigned spp) override;

        virtual void showHisto(std::string title, std::vector<float> data) override;

        virtual void updateCamera(glm::vec3 look_from, glm::vec3 look_at) override;

        void handleInput();
        void handle_mouse_move(double xPos, double yPos);
        void handle_mouse_buttons(int button, int action, int mods);
        void handle_mouse_scroll(double xoffset, double yoffset);
    };
}
