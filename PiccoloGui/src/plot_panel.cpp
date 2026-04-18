#include "plot_panel.hpp"
#include "imgui.h"
#include "implot.h"
#include <vector>

void drawPlotPanel(const TelemetryBuffer& telemetry)
{
    ImGui::TextUnformatted("Real-Time Data Plot");
    ImGui::Separator();

    if (ImPlot::BeginPlot("Motor Telemetry", ImVec2(-1, -1)))
    {
        if (!telemetry.t.empty())
        {
            std::vector<double> t(telemetry.t.begin(), telemetry.t.end());
            std::vector<double> speed(telemetry.speed.begin(), telemetry.speed.end());
            std::vector<double> current(telemetry.current.begin(), telemetry.current.end());
            std::vector<double> position(telemetry.position.begin(), telemetry.position.end());

            ImPlot::SetupAxes("Time (s)", "Value");
            ImPlot::PlotLine("Speed (rps)", t.data(), speed.data(), static_cast<int>(t.size()));
            ImPlot::PlotLine("Current (A)", t.data(), current.data(), static_cast<int>(t.size()));
            ImPlot::PlotLine("Position (deg)", t.data(), position.data(), static_cast<int>(t.size()));
        }
        ImPlot::EndPlot();
    }
}
