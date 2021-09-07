#pragma once

#include <glad/glad.h>  // include glad.h to get all required OpenGL headers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

namespace tool {
    class zoom_pixel {
    private:
        Shader shader{ "shaders/zoom_pixel/vertex.glsl", "shaders/zoom_pixel/fragment.glsl" };
        unsigned VBO, VAO, EBO;
        glm::vec3 color;
        glm::vec2 position;

        float scr_width, scr_height;
        unsigned size;

    public:
        zoom_pixel(unsigned scr_width, unsigned scr_height, unsigned size) :
            scr_width(scr_width), scr_height(scr_height), size(size),
            color(glm::vec3(1.0f, 1.0f, 1.0f)), position(scr_width / 2, scr_height / 2) {

            float vertices[] = {
                // position           // texture coords
                +1.0f, +1.0f,  0.5f,  1.0f, 1.0f,
                -1.0f, +1.0f,  0.5f,  0.0f, 1.0f,
                -1.0f, -1.0f,  0.5f,  0.0f, 0.0f,
                +1.0f, -1.0f,  0.5f,  1.0f, 0.0f
            };

            unsigned indices[] = {
                0, 1, 2,
                2, 3, 0
            };

            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            // position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }

        ~zoom_pixel() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        void set_color(glm::vec3 c) { color = c; }

        void set_position(unsigned x, unsigned y) {
            position = glm::vec2(
                2.0f * x / scr_width - 1.0f,
                1.0f - 2.0f * y / scr_height
            );
        }

        void render() {
            // render our container
            shader.use();

            glm::mat4 model{ 1.0f };
            model = glm::translate(model, glm::vec3(position, 0.0f)) *
                glm::scale(model, glm::vec3(size / scr_width, size / scr_height, 1.0f));

            shader.setMat4("model", model);
            shader.setVec3("aColor", color);

            glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            // glBindVertexArray(0); // no need to unbind it every time 
        }
    };
}