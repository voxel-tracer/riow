#pragma once

#include "imguiManager.h"

namespace tool {

    class HistoWidget : public Widget {
    private:
        std::string title;
        std::vector<float> data;

    public:
        HistoWidget(std::string title, std::vector<float> data) :
            title(title), data(data) {}

        virtual void Render() override {
            ImGui::Begin(title.c_str());
            ImGui::PlotHistogram("", &data[0], data.size(), 0, NULL, 0.0f, 1.0f, ImVec2(220, 100.0f));
            ImGui::End();
        }
    };

    class RenderModeWidget : public Widget {
    public:
        bool preview = false;
        int previewSize = 5;

        virtual void Render() override {
            ImGui::Begin("Render Settings");
            ImGui::Checkbox("preview", &preview);
            if (preview) {
                ImGui::SliderInt("preview size", &previewSize, 1, 101);
            }
            ImGui::End();
        }
    };
}