#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace tool {

    class camera {
    private:
        const float rotate_speed = 0.5f;
        const float zoom_speed = 0.5f;

        glm::vec3 look_at;
        glm::vec3 look_from;
        float dist_to_look_at;

        // around look at angles
        float x_angle = 0.0f;
        float y_angle = 0.0f;

        float mouse_last_x = 0.0f;
        float mouse_last_y = 0.0f;
        bool  mouse_left_pressed = false;

    public:
        camera(glm::vec3 look_at = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 look_from = glm::vec3(0.0f, 0.0f, -3.0f)) :
            look_at(look_at) {
            dist_to_look_at = glm::length(look_from - look_at);
            compute_rotations(look_from);

            update_camera_vectors();
        }

        glm::mat4 get_view_matrix() const {
            return glm::lookAt(glm::vec3(look_from), look_at, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        void compute_rotations(glm::vec3 look_from) {
            glm::vec3 lfn = glm::normalize(look_from - look_at);
            float xa = std::acos(std::sqrtf(lfn.x * lfn.x + lfn.z * lfn.z));
            if (lfn.y > 0) xa = -xa;
            float ya = std::acos(lfn.z / std::sqrtf(lfn.x * lfn.x + lfn.z * lfn.z));
            if (lfn.x < 0) ya = -ya;

            x_angle = glm::degrees(xa);
            y_angle = glm::degrees(ya);
        }

        void handle_mouse_move(double xPos, double yPos) {
            if (mouse_left_pressed) {
                // update camera angles
                float delta_x = xPos - mouse_last_x;
                float delta_y = yPos - mouse_last_y;

                x_angle -= delta_y * rotate_speed;
                y_angle -= delta_x * rotate_speed;

                if (x_angle > 89.0f) x_angle = 89.0f;
                if (x_angle < -89.0f) x_angle = -89.0f;

                update_camera_vectors();
            }

            mouse_last_x = xPos;
            mouse_last_y = yPos;
        }

        void handle_mouse_buttons(int button, int action, int mods) {
            // mods can be used to check if GLFW_MOD_[SHIFT|CONTROL|ALT|SUPER] was also pressed
            if (action == GLFW_PRESS) {
                if (button == GLFW_MOUSE_BUTTON_1) {// mouse left button pressed
                    mouse_left_pressed = true;
                }
            }
            if (action == GLFW_RELEASE) {
                if (button == GLFW_MOUSE_BUTTON_1) {// mouse left button released
                    mouse_left_pressed = false;
                }
            }
        }

        void handle_mouse_scroll(double xoffset, double yoffset) {
            dist_to_look_at += zoom_speed * yoffset;

            if (dist_to_look_at < 0.5f) dist_to_look_at = 0.5f;
            if (dist_to_look_at > 10.0f) dist_to_look_at = 10.0f;

            update_camera_vectors();
        }

    private:
        void update_camera_vectors() {
            glm::mat4 mat{ 1.0f };
            mat = glm::rotate(mat, glm::radians(y_angle), glm::vec3(0.0f, 1.0f, 0.0f));
            mat = glm::rotate(mat, glm::radians(x_angle), glm::vec3(1.0f, 0.0f, 0.0f));
            look_from = glm::vec3(mat * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)) * dist_to_look_at + look_at;
        }
    };

}