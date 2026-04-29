#include "telemetry_panel.hpp"
#include "imgui.h"

void drawTelemetryPanel(const TelemetryBuffer& telemetry, bool& armed, int status, bool& autoRefresh, int& refreshRateIndex, bool& loggingEnabled)
{
    ImGui::Text("Telemetry");
    ImGui::SameLine();
    if (ImGui::SmallButton(autoRefresh ? "Auto-Refresh: ON" : "Auto-Refresh: OFF"))
    {
        autoRefresh = !autoRefresh;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(loggingEnabled ? "Logging: ON" : "Logging: OFF"))
    {
        loggingEnabled = !loggingEnabled;
    }

    ImGui::Text("Refresh rate:");
    ImGui::SameLine();
    if (ImGui::RadioButton("1Hz", &refreshRateIndex, 0)) {}
    ImGui::SameLine();
    if (ImGui::RadioButton("10Hz", &refreshRateIndex, 1)) {}
    ImGui::SameLine();
    if (ImGui::RadioButton("50Hz", &refreshRateIndex, 2)) {}

    ImGui::SameLine();
    static const char* rateLabels[] = {"1Hz", "10Hz", "50Hz"};
    ImGui::Text("Current: %s", rateLabels[refreshRateIndex]);

    ImGui::Separator();

    // Fields
    if (!telemetry.busVoltage.empty())
    {
        ImGui::Text("Bus Voltage: %.2f V", telemetry.busVoltage.back());
    }
    else
    {
        ImGui::Text("Bus Voltage: -- V");
    }

    ImGui::SameLine();
    if (!telemetry.current.empty())
    {
        ImGui::Text("Motor Current: %.2f A", telemetry.current.back());
    }
    else
    {
        ImGui::Text("Motor Current: -- A");
    }

    ImGui::SameLine();
    if (!telemetry.pwmPercent.empty())
    {
        ImGui::Text("PWM %%: %.1f %%", telemetry.pwmPercent.back());
    }
    else
    {
        ImGui::Text("PWM %%: -- %%");
    }

    ImGui::SameLine();
    if (!telemetry.driverTemp.empty())
    {
        ImGui::Text("Driver Temp: %.1f °C", telemetry.driverTemp.back());
    }
    else
    {
        ImGui::Text("Driver Temp: -- °C");
    }

    ImGui::SameLine();
    if (!telemetry.motorTemp.empty())
    {
        ImGui::Text("Motor Temp: %.1f °C", telemetry.motorTemp.back());
    }
    else
    {
        ImGui::Text("Motor Temp: -- °C");
    }

    ImGui::Spacing();

    if (!telemetry.multiTurnEncoder.empty())
    {
        // Encoder returns 0-16383 for 0-360 degrees
        int encoderCounts = telemetry.multiTurnEncoder.back();
        ImGui::Text("Encoder (multi-turn): %d", encoderCounts);
    }
    else
    {
        ImGui::Text("Encoder (multi-turn): -- ");
    }

    ImGui::Spacing();
    ImGui::SameLine();
    ImVec2 indicatorPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec4 indicatorColor;
    if (status == 0) indicatorColor = ImVec4(0, 1, 0, 1); // Green
    else if (status == 1) indicatorColor = ImVec4(1, 1, 0, 1); // Yellow
    else indicatorColor = ImVec4(1, 0, 0, 1); // Red
    drawList->AddCircleFilled(ImVec2(indicatorPos.x + 10, indicatorPos.y + 15), 10, ImGui::ColorConvertFloat4ToU32(indicatorColor));
    ImGui::Dummy(ImVec2(25, 20)); // Space for the circle

    // Arm/Disarm button
    ImGui::SameLine();
    ImVec2 buttonSize(60, 60);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec4 buttonColor = armed ? ImVec4(0, 0.5, 0, 1) : ImVec4(0.5, 0, 0, 1);
    drawList->AddCircleFilled(ImVec2(pos.x + buttonSize.x / 2, pos.y + buttonSize.y / 2), buttonSize.x / 2, ImGui::ColorConvertFloat4ToU32(buttonColor));
    ImGui::InvisibleButton(armed ? "Disarm" : "Arm", buttonSize);
    if (ImGui::IsItemClicked())
    {
        armed = !armed;
        // TODO: send command to device
    }
    ImGui::SetCursorScreenPos(ImVec2(pos.x + buttonSize.x / 2 - ImGui::CalcTextSize(armed ? "Disarm" : "Arm").x / 2, pos.y + buttonSize.y / 2 - ImGui::GetTextLineHeight() / 2));
    ImGui::Text(armed ? "Disarm" : "Arm");
}