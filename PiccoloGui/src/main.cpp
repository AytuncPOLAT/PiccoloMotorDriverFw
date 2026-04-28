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
#include "serial_manager.hpp"
#include "telemetry_panel.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <implot.h>
#include <string>
#include <vector>

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

    static float leftPanelWidth = 600.0f;
    static float telemetryPanelHeight = 100.0f;
    bool autoRefresh = false;
    double lastRefreshTime = glfwGetTime();

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
            double speed = 0.0;
            double current = 0.0;
            double position = 0.0;
            double busVoltage = 0.0;
            double pwmPercent = 0.0;
            double driverTemp = 0.0;
            double motorTemp = 0.0;
            if (parseTelemetryCsv(line, speed, current, position, busVoltage, pwmPercent, driverTemp, motorTemp))
            {
                telemetry.push(now, speed, current, position, busVoltage, pwmPercent, driverTemp, motorTemp, 0.0);
            }
        }

        // Auto-refresh telemetry at 10Hz
        if (autoRefresh && serial.isConnected())
        {
            const double timeSinceRefresh = now - lastRefreshTime;
            if (timeSinceRefresh >= 0.1) // 100ms for 10Hz
            {
                lastRefreshTime = now;
                
                int32_t busVoltageRaw = 0;
                int32_t encoderRaw = 0;
                std::string readError;
                if (serial.readProperty(static_cast<uint8_t>(connectionState.deviceAddress), 
                                       Common::PROPERTY::DC_BUS_VOLTAGE, 
                                       busVoltageRaw, readError))
                {
                    // Convert raw value to voltage (assuming raw value is voltage * 100 or similar scaling)
                    double busVoltage = busVoltageRaw / 100.0;
                    
                    // Also read the multi-turn encoder
                    double encoder = 0.0;
                    std::string encoderError;
                    if (serial.readProperty(static_cast<uint8_t>(connectionState.deviceAddress), 
                                           Common::PROPERTY::MULTI_TURN_ENCODER, 
                                           encoderRaw, encoderError))
                    {
                        // Encoder returns 0-16383 for one full turn
                        // Ensure it's treated as unsigned
                        uint32_t encoderUnsigned = static_cast<uint32_t>(encoderRaw);
                        encoder = static_cast<double>(encoderUnsigned);
                    }
                    
                    // Push with last known values for other fields
                    double lastSpeed = telemetry.speed.empty() ? 0.0 : telemetry.speed.back();
                    double lastCurrent = telemetry.current.empty() ? 0.0 : telemetry.current.back();
                    double lastPosition = telemetry.position.empty() ? 0.0 : telemetry.position.back();
                    double lastPwm = telemetry.pwmPercent.empty() ? 0.0 : telemetry.pwmPercent.back();
                    double lastDrvTemp = telemetry.driverTemp.empty() ? 0.0 : telemetry.driverTemp.back();
                    double lastMotTemp = telemetry.motorTemp.empty() ? 0.0 : telemetry.motorTemp.back();
                    
                    telemetry.push(now, lastSpeed, lastCurrent, lastPosition, busVoltage, lastPwm, lastDrvTemp, lastMotTemp, encoder);
                    addLog(logs, "TX > Telemetry refresh: Bus Voltage = " + std::to_string(busVoltage) + "V, Encoder = " + std::to_string(static_cast<int32_t>(encoder)));
                }
                else if (!readError.empty())
                {
                    addLog(logs, "Telemetry refresh error: " + readError);
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
            drawJogPanel(serial, static_cast<uint8_t>(connectionState.deviceAddress), logs);
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
            drawPlotPanel(telemetry);
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, spacing));

        ImGui::BeginChild("TelemetryPanel", ImVec2(0, telemetryPanelHeight), true);
        {
            drawTelemetryPanel(telemetry, armed, status, autoRefresh);
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
