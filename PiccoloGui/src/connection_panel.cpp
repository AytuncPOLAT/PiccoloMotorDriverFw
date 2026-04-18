#include "connection_panel.hpp"
#include "imgui.h"
#include "log_utils.hpp"
#include <algorithm>

void drawConnectionPanel(ConnectionPanelState& state, SerialManager& serial, std::vector<std::string>& logs)
{
    ImGui::TextUnformatted("Serial Connection");
    ImGui::Separator();

    if (ImGui::Button("Refresh Ports"))
    {
        state.ports = serial.listAvailablePorts();
        if (state.ports.empty())
        {
            state.selectedPort = -1;
            addLog(logs, "No COM ports found.");
        }
        else
        {
            state.selectedPort = std::clamp(state.selectedPort, 0, static_cast<int>(state.ports.size()) - 1);
            addLog(logs, "Found " + std::to_string(state.ports.size()) + " COM ports.");
        }
    }

    ImGui::SameLine();
    ImGui::InputInt("Baud", &state.baudRate);
    if (state.baudRate < 1200)
    {
        state.baudRate = 1200;
    }

    const char* preview = (state.selectedPort >= 0 && state.selectedPort < static_cast<int>(state.ports.size()))
        ? state.ports[state.selectedPort].c_str()
        : "None";
    if (ImGui::BeginCombo("Detected Ports", preview))
    {
        for (int i = 0; i < static_cast<int>(state.ports.size()); ++i)
        {
            const bool isSelected = (i == state.selectedPort);
            if (ImGui::Selectable(state.ports[i].c_str(), isSelected))
            {
                state.selectedPort = i;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::InputText("Manual Port", state.manualPort.data(), state.manualPort.size());

    ImGui::InputInt("Device Address", &state.deviceAddress);
    if (state.deviceAddress < 0)
    {
        state.deviceAddress = 0;
    }
    if (state.deviceAddress > 255)
    {
        state.deviceAddress = 255;
    }

    if (ImGui::Button("Ping"))
    {
        if (!serial.isConnected())
        {
            addLog(logs, "Ping failed: not connected.");
        }
        else
        {
            std::string error;
            if (serial.sendPing(static_cast<uint8_t>(state.deviceAddress), error))
            {
                addLog(logs, "Ping OK: device " + std::to_string(state.deviceAddress));
            }
            else
            {
                addLog(logs, "Ping failed: " + error);
            }

            for (const auto& debugLine : serial.takeDebugMessages())
            {
                addLog(logs, debugLine);
            }
        }
    }

    if (!serial.isConnected())
    {
        if (ImGui::Button("Connect"))
        {
            std::string error;
            std::string targetPort = state.manualPort.data();
            if (state.selectedPort >= 0 && state.selectedPort < static_cast<int>(state.ports.size()))
            {
                targetPort = state.ports[state.selectedPort];
            }

            if (targetPort.empty())
            {
                addLog(logs, "No port selected.");
            }
            else if (serial.connect(targetPort, state.baudRate, error))
            {
                addLog(logs, "Connected to " + targetPort + " @ " + std::to_string(state.baudRate));
            }
            else
            {
                addLog(logs, "Connect failed: " + error);
            }
        }
    }
    else
    {
        if (ImGui::Button("Disconnect"))
        {
            addLog(logs, "Disconnected from " + serial.connectedPort());
            serial.disconnect();
        }
    }

    ImGui::SameLine();
    ImGui::Text("Status: %s", serial.isConnected() ? "Connected" : "Disconnected");

    ImGui::InputText("TX Message", state.txBuffer.data(), state.txBuffer.size());
    if (ImGui::Button("Send") && serial.isConnected())
    {
        std::string error;
        const std::string line(state.txBuffer.data());
        if (!line.empty())
        {
            if (serial.writeLine(line, error))
            {
                addLog(logs, "TX > " + line);
                state.txBuffer[0] = '\0';
            }
            else
            {
                addLog(logs, "TX failed: " + error);
            }
        }
    }
}
