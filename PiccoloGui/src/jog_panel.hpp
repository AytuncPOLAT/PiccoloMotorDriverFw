#pragma once

#include <cstdint>
#include <string>
#include <vector>

class SerialManager;

void drawJogPanel(SerialManager& serial, uint8_t deviceAddress, std::vector<std::string>& logs, float& jogIncrement);
