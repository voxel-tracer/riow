#include "imGuiManager.h"
#include <glm/glm.hpp>

namespace tool {
    ImGuiManager::ImGuiManager(GLFWwindow *window, const char* glsl_version) {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    ImGuiManager::~ImGuiManager() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    bool ImGuiManager::wantCaptureMouse() const {
        ImGuiIO& io = ImGui::GetIO();
        return io.WantCaptureMouse;
    }

    bool ImGuiManager::wantCaptureKeyboard() const {
        ImGuiIO& io = ImGui::GetIO();
        return io.WantCaptureKeyboard;
    }

    void ImGuiManager::render() {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        for (auto w : widgets) w->Render();

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
