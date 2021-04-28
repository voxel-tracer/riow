#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <yocto/yocto_image.h>

#include "camera.h"
#include "shader.h"
#include "scene.h"
#include "screen_texture.h"
#include "zoom_pixel.h"

namespace tool {
    class window {
    private:
        bool is2D;
        bool esc_pressed = false;

        camera cam;
        GLFWwindow* glwindow;

        unique_ptr<Shader> shader;

        std::shared_ptr<scene> scene;

        unique_ptr<screen_texture> screen;
        unique_ptr<zoom_pixel> pixel;

        void switchTo3D(bool force);
        void switchTo2D(bool force);

    public:
        window(const yocto::color_image& image, glm::vec3 look_at, glm::vec3 look_from);
        ~window();

        void set_scene(shared_ptr<tool::scene> sc) { scene = sc; }

        void render();

        void handle_input();
        void handle_mouse_move(double xPos, double yPos);
        void handle_mouse_buttons(int button, int action, int mods);
        void handle_mouse_scroll(double xoffset, double yoffset);
    };
}
