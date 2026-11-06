#pragma once

#include <cstdint>
#include <string>
#include <vector>

class SerialManager;

struct CoggingPanelState
{
    int32_t status = 0;
    int32_t progress = 0;
    int32_t error = 0;
    uint32_t nextMapIndex = 0;
    double nextStatusPollTime = 0.0;
    bool mapDownloadStarted = false;
    bool mapDownloadFailed = false;
    bool mapLoaded = false;
    std::vector<double> torqueMap;
};

void drawCoggingPanel(SerialManager& serial,
                      uint8_t deviceAddress,
                      CoggingPanelState& state,
                      double now,
                      std::vector<std::string>& logs);