#include "jog_panel.hpp"
#include "serial_manager.hpp"
#include "log_utils.hpp"
#include "imgui.h"
#include "../../AppLayer/Common/SystemData.hpp"

namespace
{
void sendJogCommand(SerialManager& serial, uint8_t deviceAddress, int selectedMode, float delta, 
                    float& currentTorque, float& currentSpeed, float& currentPosition, std::vector<std::string>& logs)
{
    std::string error;
    
    // Update the current value based on mode
    if (selectedMode == 0)
        currentTorque += delta;
    else if (selectedMode == 1)
        currentSpeed += delta;
    else if (selectedMode == 2)
        currentPosition += delta;
    
    // Send motion command - only the selected mode value is active, others are 0
    int32_t elecAngle = 0;
    int32_t torque = (selectedMode == 0) ? static_cast<int32_t>(currentTorque) : 0;
    int32_t speed = (selectedMode == 1) ? static_cast<int32_t>(currentSpeed) : 0;
    int32_t position = (selectedMode == 2) ? static_cast<int32_t>(currentPosition) : 0;
    
    if (serial.sendMotionCommand(deviceAddress, elecAngle, torque, speed, position, error))
    {
        std::string modeStr = (selectedMode == 0) ? "Torque" : (selectedMode == 1) ? "Speed" : "Position";
        float currentValue = (selectedMode == 0) ? currentTorque : (selectedMode == 1) ? currentSpeed : currentPosition;
        addLog(logs, "TX > Jog: Mode=" + modeStr + " Value=" + std::to_string(static_cast<int32_t>(currentValue)));
    }
    else if (!error.empty())
    {
        addLog(logs, "Jog send failed: " + error);
    }
}
} // namespace

void drawJogPanel(SerialManager& serial, uint8_t deviceAddress, std::vector<std::string>& logs)
{
    ImGui::Text("Jog Control");
    ImGui::Separator();

    // Control mode selection
    static int selectedMode = 0; // 0: Torque, 1: Speed, 2: Position
    ImGui::RadioButton("Torque", &selectedMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Speed", &selectedMode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Position", &selectedMode, 2);

    ImGui::Spacing();

    // Jog increment
    static float jogIncrement = 100.0f; // Adjust based on control mode
    ImGui::SliderFloat("Increment##jog", &jogIncrement, 1.0f, 1000.0f);

    ImGui::Spacing();

    // Display current command values
    static float currentTorque = 0.0f;
    static float currentSpeed = 0.0f;
    static float currentPosition = 0.0f;
    
    float selectedValue = (selectedMode == 0) ? currentTorque : (selectedMode == 1) ? currentSpeed : currentPosition;
    std::string modeStr = (selectedMode == 0) ? "Torque" : (selectedMode == 1) ? "Speed" : "Position";
    
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Current %s: %.0f", modeStr.c_str(), selectedValue);
    ImGui::Spacing();

    // Jog buttons layout: [-100x] [-10x] [-1x] [Zero] [+1x] [+10x] [+100x]
    ImVec2 buttonSize(60, 50);
    float totalWidth = buttonSize.x * 7 + ImGui::GetStyle().ItemSpacing.x * 6;
    float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f + ImGui::GetCursorPosX();

    ImGui::SetCursorPosX(startX);

    // -100x button
    if (ImGui::Button("-100x##neg100", buttonSize))
    {
        sendJogCommand(serial, deviceAddress, selectedMode, -jogIncrement * 100.0f, currentTorque, currentSpeed, currentPosition, logs);
    }

    ImGui::SameLine();

    // -10x button
    if (ImGui::Button("-10x##neg10", buttonSize))
    {
        sendJogCommand(serial, deviceAddress, selectedMode, -jogIncrement * 10.0f, currentTorque, currentSpeed, currentPosition, logs);
    }

    ImGui::SameLine();

    // -1x button
    if (ImGui::Button("-1x##neg1", buttonSize))
    {
        sendJogCommand(serial, deviceAddress, selectedMode, -jogIncrement, currentTorque, currentSpeed, currentPosition, logs);
    }

    ImGui::SameLine();

    // Zero/Home button
    if (ImGui::Button("ZERO##zero", buttonSize))
    {
        // Calculate delta needed to reach zero
        float deltaToZero = 0.0f;
        if (selectedMode == 0)
            deltaToZero = -currentTorque;
        else if (selectedMode == 1)
            deltaToZero = -currentSpeed;
        else if (selectedMode == 2)
            deltaToZero = -currentPosition;
        
        sendJogCommand(serial, deviceAddress, selectedMode, deltaToZero, currentTorque, currentSpeed, currentPosition, logs);
    }

    ImGui::SameLine();

    // +1x button
    if (ImGui::Button("+1x##pos1", buttonSize))
    {
        sendJogCommand(serial, deviceAddress, selectedMode, jogIncrement, currentTorque, currentSpeed, currentPosition, logs);
    }

    ImGui::SameLine();

    // +10x button
    if (ImGui::Button("+10x##pos10", buttonSize))
    {
        sendJogCommand(serial, deviceAddress, selectedMode, jogIncrement * 10.0f, currentTorque, currentSpeed, currentPosition, logs);
    }

    ImGui::SameLine();

    // +100x button
    if (ImGui::Button("+100x##pos100", buttonSize))
    {
        sendJogCommand(serial, deviceAddress, selectedMode, jogIncrement * 100.0f, currentTorque, currentSpeed, currentPosition, logs);
    }
}
