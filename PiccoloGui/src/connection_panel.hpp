#pragma once

#include "serial_manager.hpp"
#include <array>
#include <string>
#include <vector>

struct ConnectionPanelState
{
    std::vector<std::string> ports;
    int selectedPort = -1;
    int baudRate = 115200;
    int deviceAddress = 1;
    std::array<char, 32> manualPort = {'C', 'O', 'M', '3', '\0'};
    std::array<char, 256> txBuffer = {'\0'};
};

void drawConnectionPanel(ConnectionPanelState& state, SerialManager& serial, std::vector<std::string>& logs);
