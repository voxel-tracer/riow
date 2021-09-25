#include "window.h"
#include "tool_callbacks.h"
#include "widgets.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
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
    window::window(yocto::color_image& image, shared_ptr<tracer> tr, glm::vec3 look_at, glm::vec3 look_from) :
            pt(tr), cam(camera{ static_cast<float>(image.width) / image.height, look_at, look_from }) {
        // glfw: initialize and configure
        // ------------------------------
        if (!glfwInit()) {
            throw exception("Failed to initialize glfw");
        }

        const char* glsl_version = "#330";
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
        glfwSwapInterval(1); // enable vsync
        glfwSetWindowUserPointer(glwindow, this);
        glfwSetFramebufferSizeCallback(glwindow, framebuffer_size_callback);

        // glad: load all OpenGL function pointers
        // ---------------------------------------
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw exception("Failed to initialize GLAD");
        }

        imGuiManager = make_unique<ImGuiManager>(glwindow, glsl_version);

        // initialize shader
        shader = make_unique<Shader>("source/vren-gui/shaders/scene/vertex.glsl", "source/vren-gui/shaders/scene/fragment.glsl");
        screen = make_unique<screen_texture>(&image);

        pixel = make_unique<zoom_pixel>(image.width, image.height, 20);

        // create global transformations
        // ----------------------
        shader->use();
        shader->setMat4("projection", cam.get_projection_matrix());

        // start with pathTracer mode
        switchToPathTracer(true);
        //switchToWireFrame(true);

        // register mouse callback
        // -----------------------
        glfwSetCursorPosCallback(glwindow, mouse_callback);
        glfwSetMouseButtonCallback(glwindow, mouse_button_callback);
        glfwSetScrollCallback(glwindow, scroll_callback);
    }

    window::~window() {
        imGuiManager.release();

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
            handleInput();

            // render
            // ------
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // render our instances
            if (state == WindowState::PathTracer) {
                isRendering = spp == -1 || pt->numSamples() < spp;
                if (isRendering) {
                    if (cb)
                        pt->Render(1, cb);
                    else
                        pt->RenderParallel(1);
                    std::cerr << "\riteration " << pt->numSamples() << std::flush;
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

            imGuiManager->render();

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
glfwSwapBuffers(glwindow);
glfwPollEvents();
        }
    }

    void window::switchToPathTracer(bool force = false) {
        if (force || state != WindowState::PathTracer) {
            state = WindowState::PathTracer;
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if (cam.getChangedAndReset()) {
                auto from = cam.getLookFrom();
                auto at = cam.getLookAt();
                pt->updateCamera(from.x, from.y, from.z, at.x, at.y, at.z);
            }
        }
    }

    void window::switchToWireFrame(bool force = false) {
        if (force || state != WindowState::WireFrame) {
            state = WindowState::WireFrame;
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glEnable(GL_CULL_FACE);
        }
    }

    void window::handle_mouse_move(double xPos, double yPos) {
        if (imGuiManager->wantCaptureMouse()) return;

        if (state == WindowState::PathTracer) {
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
        if (imGuiManager->wantCaptureMouse()) return;

        if (state == WindowState::PathTracer) {
            if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_1) {
                if (canDebugPixels) {
                    unsigned spp = pt->numSamples() == 0 ? 1 : pt->numSamples();
                    debugPixel(mouse_last_x, mouse_last_y, spp);
                }
                switchToWireFrame();
            }
        }
        else {
            cam.handle_mouse_buttons(button, action, mods);
        }
    }

    void window::debugPixel(unsigned x, unsigned y, unsigned spp) {
        // generate render paths
        //auto buildSegmentsCb = std::make_shared<callback::in_out_segments_cb>();
        auto buildSegmentsCb = std::make_shared<callback::build_segments_cb>();
        auto collectHitsCb = std::make_shared<callback::collect_hits>();
        auto multiCb = std::make_unique<callback::multi>();
        multiCb->add(buildSegmentsCb);
        multiCb->add(collectHitsCb);

        pt->DebugPixel(x, y, spp, multiCb.get());

        hits = collectHitsCb->hits;

        if (ls) ls.reset();
        ls = make_unique<lines>(buildSegmentsCb->segments);

        {
            auto pcb = std::make_unique<callback::print_callback>(true);
            pt->DebugPixel(x, y, spp, pcb.get());
        }

        switchToWireFrame();
    }

    void window::handle_mouse_scroll(double xoffset, double yoffset) {
        if (imGuiManager->wantCaptureMouse()) return;

        if (state != WindowState::PathTracer) {
            cam.handle_mouse_scroll(xoffset, yoffset);
        }
    }

    void window::handleInput() {
        if (imGuiManager->wantCaptureKeyboard()) return;

        bool is_esc_pressed = glfwGetKey(glwindow, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        bool isNPressed = glfwGetKey(glwindow, GLFW_KEY_N) == GLFW_PRESS;
        bool isRPressed = glfwGetKey(glwindow, GLFW_KEY_R) == GLFW_PRESS;

        static bool r_pressed = false;

        if (state == WindowState::PathTracer) {
            if (!esc_pressed && is_esc_pressed)
                switchToWireFrame();
        }
        else {
            if (!esc_pressed && is_esc_pressed)
                switchToPathTracer();

            if (!n_pressed && isNPressed && !hits.empty()) {
                if (state == WindowState::WireFrame)
                    currentHitIdx = 0;
                else
                    currentHitIdx = (currentHitIdx + 1) % hits.size();
                state = WindowState::HitView;
                auto h = hits[currentHitIdx];
                cam.setLookAt({ h->p[0], h->p[1], h->p[2] });
            }

            if (isRPressed) {
                if (!r_pressed) {
                    cam.reset();
                }
            }
        }

        esc_pressed = is_esc_pressed;
        n_pressed   = isNPressed;
        r_pressed = isRPressed;
    }

    void window::showHisto(std::string title, std::vector<float> data) {
        auto w = make_shared<HistoWidget>(title, data);
        imGuiManager->addWidget(w);
    }

    void window::updateCamera(glm::vec3 look_from, glm::vec3 look_at) {
        cam.update(look_from, look_at);
    }
}

