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

        virtual void Render() const override {
            ImGui::Begin(title.c_str());
            ImGui::PlotHistogram("", &data[0], data.size(), 0, NULL, 0.0f, 1.0f, ImVec2(220, 100.0f));
            ImGui::End();
        }
    };
}