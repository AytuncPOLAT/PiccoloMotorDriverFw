#include "plot_panel.hpp"
#include "imgui.h"
#include "implot.h"
#include <vector>

void drawPlotPanel(TelemetryBuffer& telemetry)
{
    ImGui::TextUnformatted("Real-Time Data Plot");
    ImGui::Separator();

    static int selectedWindowIndex = 1;
    static const int windowOptions[] = {5, 10, 30, 60};

    ImGui::Text("Time window:");
    ImGui::SameLine();
    bool windowChanged = false;
    if (ImGui::RadioButton("5s", &selectedWindowIndex, 0))
    {
        windowChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("10s", &selectedWindowIndex, 1))
    {
        windowChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("30s", &selectedWindowIndex, 2))
    {
        windowChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("60s", &selectedWindowIndex, 3))
    {
        windowChanged = true;
    }

    if (windowChanged)
    {
        telemetry.setTimeWindowSeconds(static_cast<double>(windowOptions[selectedWindowIndex]));
    }

    if (!telemetry.t.empty())
    {
        // Ensure the buffer is trimmed to the current window even if the setting didn't just change.
        telemetry.trimToWindow();
    }

    if (ImPlot::BeginSubplots("Telemetry", 3, 1, ImVec2(-1, -1)))
    {
        const ImVec4 amberColor = ImVec4(1.0f, 0.75f, 0.0f, 1.0f);

        // Subplot 1: Multi-Turn Encoder
        if (ImPlot::BeginPlot("Multi-Turn Encoder"))
        {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxis(ImAxis_Y1, "Encoder (counts)", ImPlotAxisFlags_AutoFit);
            if (!telemetry.t.empty())
            {
                std::vector<double> t(telemetry.t.begin(), telemetry.t.end());
                std::vector<double> encoder(telemetry.multiTurnEncoder.begin(), telemetry.multiTurnEncoder.end());
                ImPlot::PushStyleColor(ImPlotCol_Line, amberColor);
                ImPlot::PlotLine("Encoder", t.data(), encoder.data(), static_cast<int>(t.size()));
                ImPlot::PopStyleColor();
            }
            else
            {
                ImGui::Text("Waiting for telemetry data...");
            }
            ImPlot::EndPlot();
        }

        // Subplot 2: Speed
        if (ImPlot::BeginPlot("Speed"))
        {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxis(ImAxis_Y1, "Speed (rps)", ImPlotAxisFlags_AutoFit);
            if (!telemetry.t.empty())
            {
                std::vector<double> t(telemetry.t.begin(), telemetry.t.end());
                std::vector<double> speed(telemetry.speed.begin(), telemetry.speed.end());
                ImPlot::PushStyleColor(ImPlotCol_Line, amberColor);
                ImPlot::PlotLine("Speed", t.data(), speed.data(), static_cast<int>(t.size()));
                ImPlot::PopStyleColor();
            }
            else
            {
                ImGui::Text("Waiting for telemetry data...");
            }
            ImPlot::EndPlot();
        }

        // Subplot 3: Torque
        if (ImPlot::BeginPlot("Torque"))
        {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxis(ImAxis_Y1, "Torque", ImPlotAxisFlags_AutoFit);
            if (!telemetry.t.empty())
            {
                std::vector<double> t(telemetry.t.begin(), telemetry.t.end());
                std::vector<double> torque(telemetry.torque.begin(), telemetry.torque.end());
                ImPlot::PushStyleColor(ImPlotCol_Line, amberColor);
                ImPlot::PlotLine("Torque", t.data(), torque.data(), static_cast<int>(t.size()));
                ImPlot::PopStyleColor();
            }
            else
            {
                ImGui::Text("Waiting for telemetry data...");
            }
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
}
