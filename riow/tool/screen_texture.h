#pragma once

#include <glad/glad.h>  // include glad.h to get all required OpenGL headers
#include <yocto/yocto_image.h>
#include "shader.h"

namespace tool {
    class screen_texture {
    private:
        Shader shader{ "shaders/screen_texture/vertex.glsl", "shaders/screen_texture/fragment.glsl" };
        unsigned texture;
        unsigned VBO, VAO, EBO;

        std::shared_ptr<yocto::color_image> image;

    public:
        screen_texture(std::shared_ptr<yocto::color_image> image) : image(image) {
            float vertices[] = {
                // position           // texture coords
                +1.0f, +1.0f,  0.0f,  1.0f, 0.0f,
                -1.0f, +1.0f,  0.0f,  0.0f, 0.0f,
                -1.0f, -1.0f,  0.0f,  0.0f, 1.0f,
                +1.0f, -1.0f,  0.0f,  1.0f, 1.0f
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
            // texture coord attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            // load and create a texture
            // texture 1
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            // set the texture wrapping parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            updateScreen();

            // tell opengl for each sampler to which texture unit it belongs to (only had to be done once)
            shader.use();
            shader.setInt("texture1", 0);
        }

        ~screen_texture() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        void updateScreen() {
            vector<yocto::vec4b> bytes;
            yocto::float_to_byte(bytes, image->pixels);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &bytes[0]);
        }

        glm::vec3 get_color(unsigned x, unsigned y) {
            yocto::vec4f color = yocto::get_pixel(*image, x, y);
            return glm::vec3(color.x, color.y, color.z);
        }

        void render() {
            // bind textures on corresponding texture units
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);

            // render our container
            shader.use();

            //glm::mat4 view = cam.get_view_matrix();
            //shader.setMat4("view", view);

            glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            // glBindVertexArray(0); // no need to unbind it every time 
        }
    };
}
