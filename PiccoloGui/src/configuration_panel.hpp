#pragma once

#include "app_models.hpp"
#include "serial_manager.hpp"
#include <string>
#include <vector>

void drawConfigurationPanel(std::vector<ConfigItem>& configItems,
                            uint8_t deviceAddress,
                            SerialManager& serial,
                            std::vector<std::string>& logs);
