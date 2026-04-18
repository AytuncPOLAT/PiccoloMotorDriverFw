#include "app_models.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl2.h"
#include "configuration_panel.hpp"
#include "connection_panel.hpp"
#include "imgui.h"
#include "log_panel.hpp"
#include "log_utils.hpp"
#include "plot_panel.hpp"
#include "serial_manager.hpp"
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
    bool autoScrollLogs = true;

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
            if (parseTelemetryCsv(line, speed, current, position))
            {
                telemetry.push(now, speed, current, position);
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
        const float bottomHeight = 190.0f;
        const float topHeight = std::max(100.0f, available.y - bottomHeight - spacing);
        const float leftWidth = std::max(300.0f, available.x * 0.35f);
        const float rightWidth = std::max(300.0f, available.x - leftWidth - spacing);

        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, topHeight), true);
        {
            ImGui::BeginChild("ConnectionPanel", ImVec2(0, 180), true);
            drawConnectionPanel(connectionState, serial, logs);
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0.0f, spacing));

            ImGui::BeginChild("ConfigurationPanel", ImVec2(0, 0), true);
            drawConfigurationPanel(configItems, static_cast<uint8_t>(connectionState.deviceAddress), serial, logs);
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("RightPanel", ImVec2(rightWidth, topHeight), true);
        {
            drawPlotPanel(telemetry);
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, spacing));

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
