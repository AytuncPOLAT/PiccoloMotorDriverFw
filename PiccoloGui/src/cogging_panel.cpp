#include "cogging_panel.hpp"
#include "imgui.h"
#include "implot.h"
#include "log_utils.hpp"
#include "serial_manager.hpp"
#include "SystemData.hpp"
#include <algorithm>

void drawCoggingPanel(SerialManager& serial,
                      uint8_t deviceAddress,
                      CoggingPanelState& state,
                      double now,
                      std::vector<std::string>& logs)
{
    if (serial.isConnected() && now >= state.nextStatusPollTime)
    {
        state.nextStatusPollTime = now + 0.25;
        std::string error;
        int32_t value = 0;
        if (serial.readProperty(deviceAddress, Common::PROPERTY::COGGING_CALIBRATION_STATUS, value, error))
        {
            state.status = value;
        }
        if (serial.readProperty(deviceAddress, Common::PROPERTY::COGGING_CALIBRATION_PROGRESS, value, error))
        {
            state.progress = value;
        }
        if (serial.readProperty(deviceAddress, Common::PROPERTY::COGGING_CALIBRATION_ERROR, value, error))
        {
            state.error = value;
        }

        if (state.status == static_cast<int32_t>(Common::COGGING_CALIBRATION_STATUS::COMPLETE) &&
            !state.mapDownloadStarted && !state.mapDownloadFailed)
        {
            state.torqueMap.assign(Common::COGGING_MAP_SIZE, 0.0);
            state.nextMapIndex = 0;
            state.mapDownloadStarted = true;
            state.mapLoaded = false;
        }
    }

    ImGui::TextUnformatted("Cogging Torque Map");
    ImGui::Separator();
    ImGui::TextWrapped("Calibration moves one mechanical revolution forward in 15 seconds, then returns in 15 seconds.");

    const bool canStart = serial.isConnected() &&
        state.status != static_cast<int32_t>(Common::COGGING_CALIBRATION_STATUS::RUNNING);
    if (!canStart)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Start Calibration"))
    {
        std::string error;
        if (serial.writeProperty(deviceAddress, Common::PROPERTY::DEV_CONTROL_MODE,
                                 static_cast<int32_t>(Common::CONTROLLER_TYPE::COGGING_CALIBRATION), error))
        {
            state.status = static_cast<int32_t>(Common::COGGING_CALIBRATION_STATUS::RUNNING);
            state.progress = 0;
            state.error = static_cast<int32_t>(Common::COGGING_CALIBRATION_ERROR::NONE);
            state.nextMapIndex = 0;
            state.mapDownloadStarted = false;
            state.mapDownloadFailed = false;
            state.mapLoaded = false;
            state.torqueMap.clear();
            state.nextStatusPollTime = 0.0;
            addLog(logs, "TX > Cogging calibration started (control mode 6)");
        }
        else if (!error.empty())
        {
            addLog(logs, "Cogging calibration start failed: " + error);
        }
    }
    if (!canStart)
    {
        ImGui::EndDisabled();
    }

    const char* statusText = "Idle";
    if (state.status == static_cast<int32_t>(Common::COGGING_CALIBRATION_STATUS::RUNNING))
    {
        statusText = "Collecting";
    }
    else if (state.status == static_cast<int32_t>(Common::COGGING_CALIBRATION_STATUS::COMPLETE))
    {
        statusText = state.mapLoaded ? "Complete" : "Reading map";
    }
    else if (state.status == static_cast<int32_t>(Common::COGGING_CALIBRATION_STATUS::FAILED))
    {
        statusText = "Failed";
    }
    ImGui::Text("Status: %s", statusText);

    const uint32_t totalSamples = Common::COGGING_MAP_SIZE * 2U;
    const float progress = std::clamp(static_cast<float>(state.progress) /
                                      static_cast<float>(totalSamples), 0.0f, 1.0f);
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "");
    ImGui::Text("Directional samples: %ld / %u", static_cast<long>(state.progress),
                static_cast<unsigned int>(totalSamples));

    if (state.error == static_cast<int32_t>(Common::COGGING_CALIBRATION_ERROR::INTERRUPTED))
    {
        ImGui::Text("Calibration stopped before all encoder bins were sampled.");
    }
    else if (state.error == static_cast<int32_t>(Common::COGGING_CALIBRATION_ERROR::TRAVEL_LIMIT))
    {
        ImGui::Text("Calibration failed: one direction did not visit every encoder bin.");
    }

    if (state.mapDownloadStarted && !state.mapDownloadFailed && !state.mapLoaded && serial.isConnected())
    {
        constexpr uint32_t chunksPerFrame = 4;
        for (uint32_t count = 0; count < chunksPerFrame && state.nextMapIndex < Common::COGGING_MAP_SIZE; ++count)
        {
            std::vector<int32_t> samples;
            std::string error;
            if (!serial.readCoggingMapChunk(deviceAddress, state.nextMapIndex, samples, error))
            {
                state.mapDownloadFailed = true;
                if (!error.empty())
                {
                    addLog(logs, "Cogging map download failed: " + error);
                }
                break;
            }
            for (const int32_t correction : samples)
            {
                state.torqueMap[state.nextMapIndex++] = static_cast<double>(correction);
            }
        }

        if (state.nextMapIndex == Common::COGGING_MAP_SIZE)
        {
            state.mapLoaded = true;
            addLog(logs, "RX < Cogging map download complete (4096 samples)");
        }
    }

    if (state.mapLoaded && ImPlot::BeginPlot("Cogging Compensation", ImVec2(-1.0f, -1.0f)))
    {
        ImPlot::SetupAxis(ImAxis_X1, "12-bit encoder index", ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxis(ImAxis_Y1, "Torque compensation", ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Map", state.torqueMap.data(), static_cast<int>(state.torqueMap.size()));
        ImPlot::EndPlot();
    }
}