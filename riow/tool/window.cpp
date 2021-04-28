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
    window::window(const yocto::color_image &image, glm::vec3 look_at, glm::vec3 look_from) :
        cam(camera{ look_at, look_from }) {
        // glfw: initialize and configure
        // ------------------------------
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // glfw window creation
        // --------------------
        glwindow = glfwCreateWindow(image.width, image.height, "The Tool", NULL, NULL);
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
        pixel = make_unique<zoom_pixel>(image.width, image.height, 20);

        // create global transformations
        // ----------------------
        shader->use();
        float aspect_ratio = static_cast<float>(image.width) / image.height;
        glm::mat4 projection = glm::perspective(glm::radians(20.0f), aspect_ratio, 0.1f, 100.0f);
        shader->setMat4("projection", projection);

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

    void window::render() {
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
                screen->render();
                pixel->render();
            } else {
                shader->use();

                glm::mat4 view = cam.get_view_matrix();
                shader->setMat4("view", view);

                if (scene)
                    scene->render(*shader);
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
    }

    void window::handle_mouse_buttons(int button, int action, int mods) {
        if (is2D) {
            // clicking anywhere will switch to the 3D view
            if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_1) {
                switchTo3D();
            }
        } else {
            cam.handle_mouse_buttons(button, action, mods);
        }
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

