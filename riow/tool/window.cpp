#include "window.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    tool::window* w = (tool::window*)glfwGetWindowUserPointer(window);
    if (w) {
        w->handle_input();
    }
}

void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
    tool::window* w = (tool::window*)glfwGetWindowUserPointer(window);
    if (w) {
        w->handle_mouse_move(xPos, yPos);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    tool::window* w = (tool::window*)glfwGetWindowUserPointer(window);
    if (w) {
        w->handle_mouse_buttons(button, action, mods);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    tool::window* w = (tool::window*)glfwGetWindowUserPointer(window);
    if (w) {
        w->handle_mouse_scroll(xoffset, yoffset);
    }
}

namespace tool {
    window::window(std::shared_ptr<yocto::color_image> image, shared_ptr<tracer> tr, glm::vec3 look_at, glm::vec3 look_from) :
            pt(tr), cam(camera{ static_cast<float>(image->width) / image->height, look_at, look_from }) {
        // glfw: initialize and configure
        // ------------------------------
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // glfw window creation
        // --------------------
        glwindow = glfwCreateWindow(image->width, image->height, "The Tool", NULL, NULL);
        if (glwindow == NULL) {
            glfwTerminate();
            throw exception("Failed to create GLFW window");
        }
        glfwMakeContextCurrent(glwindow);
        glfwSetWindowUserPointer(glwindow, this);
        glfwSetFramebufferSizeCallback(glwindow, framebuffer_size_callback);

        // glad: load all OpenGL function pointers
        // ---------------------------------------
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw exception("Failed to initialize GLAD");
        }

        // initialize shader
        shader = make_unique<Shader>("shaders/scene/vertex.glsl", "shaders/scene/fragment.glsl");
        screen = make_unique<screen_texture>(image);

        pixel = make_unique<zoom_pixel>(image->width, image->height, 20);

        // create global transformations
        // ----------------------
        shader->use();
        shader->setMat4("projection", cam.get_projection_matrix());

        // start with 3D mode enabled
        switchTo2D(true);

        // register mouse callback
        // -----------------------
        glfwSetCursorPosCallback(glwindow, mouse_callback);
        glfwSetMouseButtonCallback(glwindow, mouse_button_callback);
        glfwSetScrollCallback(glwindow, scroll_callback);
    }

    window::~window() {
        // glfw: terminate, clearing all previously allocated GLFW resources
        // -----------------------------------------------------------------
        glfwTerminate();
    }

    void window::render(int spp) {
        // render loop
        // -----------
        while (!glfwWindowShouldClose(glwindow)) {
            // input
            // -----
            processInput(glwindow);

            // render
            // ------
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // render our instances
            if (is2D) {
                isRendering = spp == -1 || pt->numIterations() < spp;
                if (isRendering) {
                    pt->RenderIterationParallel();
                    std::cerr << "\riteration " << pt->numIterations() << std::flush;
                    screen->updateScreen();
                }
                screen->render();
                if (!isRendering && pixel) pixel->render();
            } else {
                shader->use();

                glm::mat4 view = cam.get_view_matrix();
                shader->setMat4("view", view);

                if (scene) scene->render(*shader);
                if (ls) ls->render(cam);
            }

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwSwapBuffers(glwindow);
            glfwPollEvents();
        }
    }

    void window::switchTo2D(bool force = false) {
        if (force || !is2D) {
            is2D = true;
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if (cam.getChangedAndReset()) {
                auto from = cam.getLookFrom();
                auto at = cam.getLookAt();
                pt->updateCamera(from.x, from.y, from.z, at.x, at.y, at.z);
            }
        }
    }

    void window::switchTo3D(bool force = false) {
        if (force || is2D) {
            is2D = false;
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
    }

    void window::handle_mouse_move(double xPos, double yPos) {
        if (is2D) {
            if (pixel) {
                pixel->set_position(xPos, yPos);
                pixel->set_color(screen->get_color(xPos, yPos));
            }
        }

        cam.handle_mouse_move(xPos, yPos);
        mouse_last_x = xPos;
        mouse_last_y = yPos;
    }

    void window::handle_mouse_buttons(int button, int action, int mods) {
        if (is2D) {
            if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_1) {
                if (!isRendering) debugPixel(mouse_last_x, mouse_last_y);
                switchTo3D();
            }
        } else {
            cam.handle_mouse_buttons(button, action, mods);
        }
    }

    void window::debugPixel(unsigned x, unsigned y) {
        // generate render paths
        shared_ptr<callback::build_segments_cb> cb = std::make_shared<callback::build_segments_cb>();
        pt->DebugPixel(x, y, cb);

        if (ls) ls.reset();
        ls = make_unique<lines>(cb->segments);

        switchTo3D();
    }

    void window::handle_mouse_scroll(double xoffset, double yoffset) {
        if (!is2D) {
            cam.handle_mouse_scroll(xoffset, yoffset);
        }
    }

    void window::handle_input() {
        bool is_esc_pressed = glfwGetKey(glwindow, GLFW_KEY_ESCAPE) == GLFW_PRESS;

        if (is2D) {
            if (!esc_pressed && is_esc_pressed)
                glfwSetWindowShouldClose(glwindow, GL_TRUE);
        } else {
            if (!esc_pressed && is_esc_pressed)
                switchTo2D();
        }

        esc_pressed = is_esc_pressed;
    }
}

