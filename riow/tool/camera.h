#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

namespace tool {

    class camera {
    private:
        const float rotate_speed = 0.5f;
        const float zoom_speed = 0.1f;
        const float min_dist = 0.000001f;
        const float max_dist = 100000.0f;
        const glm::vec3 initial_look_at;
        const glm::vec3 initial_look_from;

        float aspect_ratio;
        glm::vec3 look_at;
        glm::vec3 look_from;
        float dist_to_look_at;

        // around look at angles
        float x_angle = 0.0f;
        float y_angle = 0.0f;

        float mouse_last_x = 0.0f;
        float mouse_last_y = 0.0f;
        bool  mouse_left_pressed = false;

        bool changed = false;

    public:
        camera(float ar, glm::vec3 look_at = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 look_from = glm::vec3(0.0f, 0.0f, -3.0f)) :
            aspect_ratio(ar), initial_look_at(look_at), initial_look_from(look_from) {
            update(look_from, look_at);
        }

        void update(glm::vec3 from, glm::vec3 at) {
            look_from = from;
            look_at = at;
            dist_to_look_at = glm::length(look_from - look_at);
            compute_rotations(look_from);
            update_camera_vectors();
        }

        glm::mat4 get_view_matrix() const {
            return glm::lookAt(glm::vec3(look_from), look_at, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::mat4 get_projection_matrix() const {
            return glm::perspective(glm::radians(20.0f), aspect_ratio, 0.01f, 100.0f);
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

                    // print camera position
                    std::cerr << "cam.look_from = (" << look_from.x << ", " << look_from.y << ", " << look_from.z << ")\n";
                }
            }
        }

        void handle_mouse_scroll(double xoffset, double yoffset) {
            if (yoffset > 0) // zoom in
                dist_to_look_at *= 1.0 + zoom_speed;
            else // zoom out
                dist_to_look_at *= 1.0 - zoom_speed;

            if (dist_to_look_at < min_dist) dist_to_look_at = min_dist;
            if (dist_to_look_at > max_dist) dist_to_look_at = max_dist;

            update_camera_vectors();
        }

        glm::vec3 getLookAt() const { return look_at; }
        glm::vec3 getLookFrom() const { return look_from; }

        void setLookAt(glm::vec3 at) {
            look_at = at;
            update_camera_vectors();
        }

        void reset() {
            update(initial_look_from, initial_look_at);
        }

        bool getChangedAndReset() {
            bool c = changed;
            changed = false;
            return c;
        }

    private:
        void update_camera_vectors() {
            glm::mat4 mat{ 1.0f };
            mat = glm::rotate(mat, glm::radians(y_angle), glm::vec3(0.0f, 1.0f, 0.0f));
            mat = glm::rotate(mat, glm::radians(x_angle), glm::vec3(1.0f, 0.0f, 0.0f));
            look_from = glm::vec3(mat * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)) * dist_to_look_at + look_at;

            changed = true;
        }
    };

}
