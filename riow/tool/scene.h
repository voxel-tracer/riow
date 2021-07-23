#pragma once

#include <yocto/yocto_shape.h>
#include <yocto/yocto_scene.h>

#include <iostream>

namespace tool {
    class shape {
    private:
        unsigned VBO, VAO, EBO;
        unsigned num_triangles;

    public:
        shape(const yocto::scene_shape& s) {
            num_triangles = s.triangles.size();

            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER,
                sizeof(float) * 3 * s.positions.size(), &s.positions[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                sizeof(int) * 3 * num_triangles, &s.triangles[0], GL_STATIC_DRAW);

            // position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }

        ~shape() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        void render() const {
            glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
            glDrawElements(GL_TRIANGLES, num_triangles * 3, GL_UNSIGNED_INT, 0);
        }
    };

    struct instance {
        int shape;
        glm::mat4 model;
    };

    class scene {
    private:
        vector<unique_ptr<shape>> shapes;
        vector<instance> instances;

    public:
        scene(const yocto::scene_model& yshape) {
            for (const auto& s : yshape.shapes)
                shapes.push_back(make_unique<shape>(s));
            for (const auto& instance : yshape.instances) {
                // compute GL model transform
                auto mat = yocto::frame_to_mat(instance.frame);
                glm::mat4 model{
                    mat.x.x, mat.x.y, mat.x.z, mat.x.w,
                    mat.y.x, mat.y.y, mat.y.z, mat.y.w,
                    mat.z.x, mat.z.y, mat.z.z, mat.z.w,
                    mat.w.x, mat.w.y, mat.w.z, mat.w.w
                };

                instances.push_back({ instance.shape, model });
            }
        }

        // this is a bit weird as this class expects a specific implementation of shader that allows setting a model
        // transform for each shape
        void render(Shader& shader) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            for (const auto& instance : instances) {
                shader.setMat4("model", instance.model);
                shader.setVec3("color", glm::vec3(1.0, 0.5, 0.5));
                shapes[instance.shape]->render();
            }
            glDisable(GL_BLEND);
        }
    };

}
