#include "app_models.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl2.h"
#include "configuration_panel.hpp"
#include "connection_panel.hpp"
#include "imgui.h"
#include "jog_panel.hpp"
#include "log_panel.hpp"
#include "log_utils.hpp"
#include "plot_panel.hpp"
#include "cartesian_panel.hpp"
#include "serial_manager.hpp"
#include "telemetry_panel.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <implot.h>
#include <string>
#include <vector>
#include <cstdio>

int main()
{
    if (!glfwInit())
    {
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1600, 900, "Servo Driver Configurator", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    SerialManager serial;
    std::vector<ConfigItem> configItems = buildConfigItemsFromPropertyEnum();
    TelemetryBuffer telemetry;

    std::vector<std::string> logs;
    bool armed = false;
    int status = 0; // 0: no error, 1: warning, 2: error
    bool autoScrollLogs = true;
    extern bool loggingEnabled;
    float cartesianJogIncrement = 100.0f;

    static float leftPanelWidth = 600.0f;
    static float telemetryPanelHeight = 100.0f;
    bool autoRefresh = false;
    int refreshRateIndex = 1; // 0: 1Hz, 1: 10Hz, 2: 50Hz
    double lastSlowRefreshTime = glfwGetTime();
    double lastFastRefreshTime = glfwGetTime();
    double lastBusVoltage = 0.0;
    double lastEncoder = 0.0;
    double lastSpeed = 0.0;
    double lastTorque = 0.0;

    ConnectionPanelState connectionState;
    connectionState.ports = serial.listAvailablePorts();
    connectionState.selectedPort = connectionState.ports.empty() ? -1 : 0;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const double now = glfwGetTime();

        std::string serialError;
        const auto incoming = serial.pollLines(serialError);
        for (const auto& debugLine : serial.takeDebugMessages())
        {
            addLog(logs, debugLine);
        }
        if (!serialError.empty())
        {
            addLog(logs, "SERIAL ERROR: " + serialError);
        }

        for (const std::string& line : incoming)
        {
            addLog(logs, "RX < " + line);
            double encoder = 0.0;
            double speed = 0.0;
            double torque = 0.0;
            double busVoltage = 0.0;
            double pwmPercent = 0.0;
            double driverTemp = 0.0;
            double motorTemp = 0.0;
            if (parseTelemetryCsv(line, encoder, speed, torque, busVoltage, pwmPercent, driverTemp, motorTemp))
            {
                telemetry.push(now, speed, 0.0, 0.0, busVoltage, pwmPercent, driverTemp, motorTemp, encoder, torque);
            }
        }

        // Slow path: 1Hz for DC bus voltage
        if (autoRefresh && serial.isConnected())
        {
            const double timeSinceSlow = now - lastSlowRefreshTime;
            if (timeSinceSlow >= 1.0)
            {
                lastSlowRefreshTime = now;

                int32_t busVoltageRaw = 0;
                std::string readError;
                if (serial.readProperty(static_cast<uint8_t>(connectionState.deviceAddress),
                                       Common::PROPERTY::DC_BUS_VOLTAGE,
                                       busVoltageRaw, readError))
                {
                    lastBusVoltage = busVoltageRaw / 100.0;

                    // Push with updated busVoltage, last encoder
                    double lastSpeed = telemetry.speed.empty() ? 0.0 : telemetry.speed.back();
                    double lastCurrent = telemetry.current.empty() ? 0.0 : telemetry.current.back();
                    double lastPosition = telemetry.position.empty() ? 0.0 : telemetry.position.back();
                    double lastPwm = telemetry.pwmPercent.empty() ? 0.0 : telemetry.pwmPercent.back();
                    double lastDrvTemp = telemetry.driverTemp.empty() ? 0.0 : telemetry.driverTemp.back();
                    double lastMotTemp = telemetry.motorTemp.empty() ? 0.0 : telemetry.motorTemp.back();

                    telemetry.push(now, lastSpeed, lastCurrent, lastPosition, lastBusVoltage, lastPwm, lastDrvTemp, lastMotTemp, lastEncoder, lastTorque);
                    addLog(logs, "TX > Slow telemetry refresh: Bus Voltage = " + std::to_string(lastBusVoltage) + "V");
                }
                else if (!readError.empty())
                {
                    addLog(logs, "Slow telemetry refresh error: " + readError);
                }
            }
        }

        // Fast path: selected frequency for encoder
        if (autoRefresh && serial.isConnected())
        {
            static const int refreshRates[] = {1, 10, 50};
            const double refreshInterval = 1.0 / static_cast<double>(refreshRates[refreshRateIndex]);
            const double timeSinceFast = now - lastFastRefreshTime;
            if (timeSinceFast >= refreshInterval)
            {
                lastFastRefreshTime = now;

                int32_t encoderRaw = 0;
                int32_t speedRaw = 0;
                int32_t torqueRaw = 0;
                int32_t unusedData3 = 0;
                std::string encoderError;
                if (serial.readProperty(static_cast<uint8_t>(connectionState.deviceAddress),
                                       Common::PROPERTY::MOTION_TELEMETRY,
                                       encoderRaw, speedRaw, torqueRaw, unusedData3, encoderError))
                {
                    lastEncoder = static_cast<double>(encoderRaw);
                    lastSpeed = static_cast<double>(speedRaw);
                    lastTorque = static_cast<double>(torqueRaw);

                    // Push with updated encoder, speed, torque, and last busVoltage
                    double lastCurrent = telemetry.current.empty() ? 0.0 : telemetry.current.back();
                    double lastPosition = telemetry.position.empty() ? 0.0 : telemetry.position.back();
                    double lastPwm = telemetry.pwmPercent.empty() ? 0.0 : telemetry.pwmPercent.back();
                    double lastDrvTemp = telemetry.driverTemp.empty() ? 0.0 : telemetry.driverTemp.back();
                    double lastMotTemp = telemetry.motorTemp.empty() ? 0.0 : telemetry.motorTemp.back();

                    telemetry.push(now, lastSpeed, lastCurrent, lastPosition, lastBusVoltage, lastPwm, lastDrvTemp, lastMotTemp, lastEncoder, lastTorque);
                    addLog(logs, "TX > Fast telemetry refresh: Encoder = " + std::to_string(static_cast<int32_t>(lastEncoder)) + ", Speed = " + std::to_string(static_cast<int32_t>(lastSpeed)) + ", Torque = " + std::to_string(static_cast<int32_t>(lastTorque)));
                }
                else if (!encoderError.empty())
                {
                    addLog(logs, "Fast telemetry refresh error: " + encoderError);
                }
            }
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        const ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("Servo Driver Tool", nullptr, rootFlags);

        ImVec2 available = ImGui::GetContentRegionAvail();
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float topHeight = std::max(100.0f, available.y - telemetryPanelHeight - 130.0f - spacing * 2);
        const float rightWidth = std::max(300.0f, available.x - leftPanelWidth - spacing);

        ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth, topHeight), true);
        {
            ImGui::BeginChild("ConnectionPanel", ImVec2(0, 200), true);
            drawConnectionPanel(connectionState, serial, logs);
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0.0f, spacing));

            ImGui::BeginChild("JogPanel", ImVec2(0, 180), true);
            drawJogPanel(serial, static_cast<uint8_t>(connectionState.deviceAddress), logs, cartesianJogIncrement);
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0.0f, spacing));

            ImGui::BeginChild("ConfigurationPanel", ImVec2(0, 0), true);
            drawConfigurationPanel(configItems, static_cast<uint8_t>(connectionState.deviceAddress), serial, logs);
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Horizontal splitter
        ImGui::InvisibleButton("HSplitter", ImVec2(4, topHeight));
        if (ImGui::IsItemActive())
        {
            leftPanelWidth += ImGui::GetIO().MouseDelta.x;
            leftPanelWidth = std::max(200.0f, std::min(leftPanelWidth, available.x - 200.0f));
        }
        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        ImGui::SameLine();

        ImGui::BeginChild("RightPanel", ImVec2(rightWidth, topHeight), true);
        {
            if (ImGui::BeginTabBar("RightTabs"))
            {
                if (ImGui::BeginTabItem("Plots"))
                {
                    drawPlotPanel(telemetry);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Cartesian"))
                {
                    drawCartesianPanel([&serial, &logs, &cartesianJogIncrement](float x, float y, float z, float angle1, float angle2){
                        static bool first = true;
                        static float prevAngle1 = 0.0f;
                        static float prevAngle2 = 0.0f;

                        if (first || angle1 != prevAngle1)
                        {
                            int32_t a1 = static_cast<int32_t>(-angle1 * cartesianJogIncrement * 2 * 180.0f / 3.14159265f);
                            std::string err;
                            serial.sendRealtimeCommand(static_cast<uint8_t>(2), RealtimeCommandType::MOTION_POS_COMMAND, a1, err);
                            if (!err.empty()) addLog(logs, std::string("Cartesian link1 send failed: ") + err);
                            prevAngle1 = angle1;
                        }

                        if (first || angle2 != prevAngle2)
                        {
                            int32_t a2 = static_cast<int32_t>((angle2 + angle1) * cartesianJogIncrement * 2 * 180.0f / 3.14159265f);
                            std::string err;
                            serial.sendRealtimeCommand(static_cast<uint8_t>(3), RealtimeCommandType::MOTION_POS_COMMAND, a2, err);
                            if (!err.empty()) addLog(logs, std::string("Cartesian link2 send failed: ") + err);
                            prevAngle2 = angle2;
                        }

                        first = false;

                        std::printf("Cartesian: x=%.2f y=%.2f z=%.2f θ1=%.1f° θ2=%.1f°\n", x, y, z, angle1 * 180.0f / 3.14159265f, angle2 * 180.0f / 3.14159265f);
                    });
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, spacing));

        ImGui::BeginChild("TelemetryPanel", ImVec2(0, telemetryPanelHeight), true);
        {
            drawTelemetryPanel(telemetry, armed, status, autoRefresh, refreshRateIndex, loggingEnabled, serial, connectionState);
        }
        ImGui::EndChild();

        // Vertical splitter
        ImGui::InvisibleButton("VSplitter", ImVec2(available.x, 4));
        if (ImGui::IsItemActive())
        {
            telemetryPanelHeight += ImGui::GetIO().MouseDelta.y;
            telemetryPanelHeight = std::max(30.0f, std::min(telemetryPanelHeight, available.y - topHeight - 50.0f - spacing * 2));
        }
        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }

        ImGui::BeginChild("LogPanel", ImVec2(0, 0), true);
        {
            drawLogPanel(logs, autoScrollLogs);
        }
        ImGui::EndChild();

        // Draw arm/disarm button and status indicator at bottom right
        {
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Button and indicator dimensions
            ImVec2 buttonSize(60, 60);
            float indicatorRadius = 10.0f;
            float padding = 10.0f;

            // Position at bottom right
            ImVec2 buttonPos = ImVec2(windowPos.x + windowSize.x - buttonSize.x - padding,
                                      windowPos.y + windowSize.y - buttonSize.y - padding);
            ImVec2 indicatorPos = ImVec2(buttonPos.x - indicatorRadius * 2 - padding,
                                         buttonPos.y + buttonSize.y / 2);

            // Draw status indicator circle
            ImVec4 indicatorColor;
            if (status == 0) indicatorColor = ImVec4(0, 1, 0, 1); // Green
            else if (status == 1) indicatorColor = ImVec4(1, 1, 0, 1); // Yellow
            else indicatorColor = ImVec4(1, 0, 0, 1); // Red
            drawList->AddCircleFilled(indicatorPos, indicatorRadius, ImGui::ColorConvertFloat4ToU32(indicatorColor));

            // Draw arm/disarm button
            ImVec4 buttonColor = armed ? ImVec4(0, 0.5, 0, 1) : ImVec4(0.5, 0, 0, 1);
            drawList->AddCircleFilled(ImVec2(buttonPos.x + buttonSize.x / 2, buttonPos.y + buttonSize.y / 2),
                                      buttonSize.x / 2, ImGui::ColorConvertFloat4ToU32(buttonColor));

            // Create invisible button for interaction
            ImGui::SetCursorScreenPos(buttonPos);
            ImGui::InvisibleButton(armed ? "Disarm##btn" : "Arm##btn", buttonSize);
            if (ImGui::IsItemClicked())
            {
                armed = !armed;
                // TODO: send command to device
            }

            // Draw button text
            const char* buttonText = armed ? "Disarm" : "Arm";
            ImVec2 textSize = ImGui::CalcTextSize(buttonText);
            drawList->AddText(ImVec2(buttonPos.x + buttonSize.x / 2 - textSize.x / 2,
                                     buttonPos.y + buttonSize.y / 2 - textSize.y / 2),
                             ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), buttonText);
        }

        ImGui::End();

        ImGui::Render();
        int displayW = 0;
        int displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);

        glViewport(0, 0, displayW, displayH);
        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    serial.disconnect();

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
