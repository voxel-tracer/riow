#pragma once

#include <vector>

#include "shader.h"
#include "tracer_data.h"

namespace tool {
    class lines {
    private:
        unsigned numverts;
        unsigned VBO, VAO;

    public:
        lines(const std::vector<path_segment> segments) {
            vector<vec3f> vs{};
            for (auto& s : segments) {
                vs.push_back(s.s);
                vs.push_back(s.e);
            }

            numverts = vs.size();

            // convert segments to a regular vec3f array

            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, numverts * sizeof(yocto::vec3f), &vs[0], GL_STATIC_DRAW);

            // position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
            // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
            glBindVertexArray(0);
        }

        ~lines() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }

        void render(Shader& shader) {
            shader.use();
            shader.setMat4("model", glm::mat4(1.0f));
            shader.setVec3("color", glm::vec3(1.0, 1.0, 1.0));

            glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
            glDrawArrays(GL_LINES, 0, numverts);
        }

    };

}