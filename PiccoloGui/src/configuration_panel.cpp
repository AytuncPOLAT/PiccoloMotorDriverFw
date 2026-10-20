#include "configuration_panel.hpp"
#include "imgui.h"
#include "log_utils.hpp"
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace
{
    bool isPropertyWritable(Common::PROPERTY property)
    {
        switch (property)
        {
            case Common::PROPERTY::FLASH_MAGIC:
            case Common::PROPERTY::DC_BUS_VOLTAGE:
            case Common::PROPERTY::GATE_DRIVER_TYPE:
            case Common::PROPERTY::GATE_CONFIG_STATUS:
                return false;
            default:
                return true;
        }
    }

    void readConfigItem(ConfigItem& item, uint8_t deviceAddress, SerialManager& serial, std::vector<std::string>& logs)
    {
        if (!serial.isConnected())
        {
            addLog(logs, std::string("Read failed for ") + item.label + ": not connected.");
            return;
        }

        int32_t readValue = item.value;
        std::string error;
        if (serial.readProperty(deviceAddress, item.property, readValue, error))
        {
            item.value = readValue;
            addLog(logs, std::string("Read ") + item.label + " = " + std::to_string(item.value));
            return;
        }

        addLog(logs, std::string("Read failed for ") + item.label + ": " + error);
    }

    void writeConfigItem(const ConfigItem& item,
                         uint8_t deviceAddress,
                         SerialManager& serial,
                         std::vector<std::string>& logs)
    {
        if (!serial.isConnected())
        {
            addLog(logs, std::string("Write failed for ") + item.label + ": not connected.");
            return;
        }

        if (!isPropertyWritable(item.property))
        {
            addLog(logs, std::string("Write skipped for ") + item.label + ": property is read-only.");
            return;
        }

        std::string error;
        if (Common::IsGateDriverSetting(item.property))
        {
            int32_t driverType = 0;
            if (!serial.readProperty(deviceAddress, Common::PROPERTY::GATE_DRIVER_TYPE, driverType, error))
            {
                addLog(logs, std::string("Gate driver detection failed: ") + error);
                return;
            }
            if (driverType != 8353)
            {
                addLog(logs, "Gate configuration requires DRV8353S firmware with gate configuration support.");
                return;
            }
        }
        if (serial.writeProperty(deviceAddress, item.property, item.value, error))
        {
            if (Common::IsGateDriverSetting(item.property))
            {
                int32_t status = 0;
                int32_t readValue = 0;
                if (!serial.readProperty(deviceAddress, Common::PROPERTY::GATE_CONFIG_STATUS, status, error) ||
                    !serial.readProperty(deviceAddress, item.property, readValue, error))
                {
                    addLog(logs, std::string("Gate configuration verification failed: ") + error);
                    return;
                }
                if (status != static_cast<int32_t>(Common::GateConfigStatus::OK) || readValue != item.value)
                {
                    const char* reason = status == 1 ? "unsupported driver" :
                        status == 2 ? "invalid value" : status == 3 ? "SPI/readback failure" : "value mismatch";
                    addLog(logs, std::string("Write failed for ") + item.label + ": " + reason);
                    return;
                }
            }
            addLog(logs, std::string("Wrote ") + item.label + " = " + std::to_string(item.value));
            return;
        }

        addLog(logs, std::string("Write failed for ") + item.label + ": " + error);
    }

    void saveConfigToJson(const std::vector<ConfigItem>& configItems, const std::string& filename, std::vector<std::string>& logs)
    {
        std::ofstream file(filename);
        if (!file.is_open())
        {
            addLog(logs, std::string("Failed to open file for saving: ") + filename);
            return;
        }

        file << "{\n";
        file << "  \"configuration\": [\n";

        for (size_t i = 0; i < configItems.size(); ++i)
        {
            const auto& item = configItems[i];
            file << "    {\n";
            file << "      \"property_id\": " << static_cast<int>(item.property) << ",\n";
            file << "      \"label\": \"" << item.label << "\",\n";
            file << "      \"value\": " << item.value << "\n";
            file << "    }";
            if (i < configItems.size() - 1)
                file << ",";
            file << "\n";
        }

        file << "  ]\n";
        file << "}\n";
        file.close();

        addLog(logs, std::string("Configuration saved to ") + filename);
    }

    void loadConfigFromJson(std::vector<ConfigItem>& configItems, const std::string& filename, std::vector<std::string>& logs)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            addLog(logs, std::string("Failed to open file for loading: ") + filename);
            return;
        }

        std::string line;
        size_t loadedCount = 0;

        // Simple JSON parsing - look for "property_id" and "value" pairs
        std::map<int, int32_t> loadedValues;
        int currentPropertyId = -1;

        while (std::getline(file, line))
        {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos)
                continue;

            // Parse property_id
            if (line.find("\"property_id\"") != std::string::npos)
            {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos)
                {
                    std::string valueStr = line.substr(colonPos + 1);
                    // Remove trailing comma and whitespace
                    size_t commaPos = valueStr.find(',');
                    if (commaPos != std::string::npos)
                        valueStr = valueStr.substr(0, commaPos);
                    try
                    {
                        currentPropertyId = std::stoi(valueStr);
                    }
                    catch (...)
                    {
                        currentPropertyId = -1;
                    }
                }
            }

            // Parse value
            if (line.find("\"value\"") != std::string::npos && currentPropertyId >= 0)
            {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos)
                {
                    std::string valueStr = line.substr(colonPos + 1);
                    // Remove trailing comma and whitespace
                    size_t commaPos = valueStr.find(',');
                    if (commaPos != std::string::npos)
                        valueStr = valueStr.substr(0, commaPos);
                    try
                    {
                        int32_t value = std::stoi(valueStr);
                        loadedValues[currentPropertyId] = value;
                        currentPropertyId = -1;
                    }
                    catch (...)
                    {
                        currentPropertyId = -1;
                    }
                }
            }
        }

        file.close();

        // Apply loaded values to config items
        for (auto& item : configItems)
        {
            int propId = static_cast<int>(item.property);
            auto it = loadedValues.find(propId);
            if (it != loadedValues.end())
            {
                item.value = it->second;
                ++loadedCount;
            }
        }

        addLog(logs, std::string("Configuration loaded from ") + filename + ": " + std::to_string(loadedCount) + " values loaded.");
    }
    void drawGateSetting(ConfigItem& item, const std::string& label)
    {
        const bool source = item.property == Common::PROPERTY::GATE_HS_SOURCE ||
                            item.property == Common::PROPERTY::GATE_LS_SOURCE;
        const bool sink = item.property == Common::PROPERTY::GATE_HS_SINK ||
                          item.property == Common::PROPERTY::GATE_LS_SINK;
        const int sourceMa[] = {50, 100, 150, 300, 350, 400, 450, 550, 600, 650, 700, 850, 900, 950, 1000};
        const int sinkMa[] = {100, 200, 300, 600, 700, 800, 900, 1100, 1200, 1300, 1400, 1700, 1800, 1900, 2000};
        const int gains[] = {5, 10, 20, 40};
        const int timesNs[] = {500, 1000, 2000, 4000};
        const int count = source || sink ? 15 : 4;
        auto optionValue = [&](int index) {
            return source || sink ? index + 1 :
                item.property == Common::PROPERTY::CURRENT_AMPLIFIER_GAIN ? gains[index] : index;
        };
        auto optionLabel = [&](int index) {
            const int value = source ? sourceMa[index] : sink ? sinkMa[index] :
                item.property == Common::PROPERTY::CURRENT_AMPLIFIER_GAIN ? gains[index] : timesNs[index];
            return std::to_string(value) + (source || sink ? " mA" :
                item.property == Common::PROPERTY::CURRENT_AMPLIFIER_GAIN ? " V/V" : " ns");
        };
        std::string preview = "Unknown (" + std::to_string(item.value) + ")";
        for (int index = 0; index < count; ++index)
            if (item.value == optionValue(index))
                preview = optionLabel(index);
        if (ImGui::BeginCombo(label.c_str(), preview.c_str()))
        {
            for (int index = 0; index < count; ++index)
            {
                if (ImGui::Selectable(optionLabel(index).c_str(), item.value == optionValue(index)))
                    item.value = optionValue(index);
            }
            ImGui::EndCombo();
        }
    }
} // namespace

void drawConfigurationPanel(std::vector<ConfigItem>& configItems,
                            uint8_t deviceAddress,
                            SerialManager& serial,
                            std::vector<std::string>& logs)
{
    ImGui::TextUnformatted("Configuration (SystemData PROPERTY)");
    ImGui::Separator();

    // Group items by category
    std::map<ConfigCategory, std::vector<ConfigItem*>> itemsByCategory;
    for (auto& item : configItems)
    {
        itemsByCategory[item.category].push_back(&item);
    }

    // Render each category as a collapsing header
    for (int catIdx = 0; catIdx < 4; ++catIdx)
    {
        ConfigCategory cat = static_cast<ConfigCategory>(catIdx);
        auto it = itemsByCategory.find(cat);
        if (it == itemsByCategory.end())
            continue;

        if (ImGui::CollapsingHeader(toCategoryLabel(cat), ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (cat == ConfigCategory::GATE_DRIVER)
            {
                ImGui::TextWrapped("DRV8353S only. W applies immediately; Store saves to flash. "
                                   "Changing drive strength or gain while driving can disturb motor control.");
                ImGui::TextWrapped("Status: 0=OK, 1=unsupported, 2=invalid value, 3=SPI/readback failure.");
            }
            for (auto itemPtr : it->second)
            {
                const int propertyId = static_cast<int>(itemPtr->property);
                const std::string idSuffix = "##prop_" + std::to_string(propertyId);

                ImGui::PushID(propertyId);
                ImGui::SetNextItemWidth(150.0f);
                if (Common::IsGateDriverSetting(itemPtr->property))
                {
                    drawGateSetting(*itemPtr, std::string(itemPtr->label) + idSuffix);
                }
                else if (itemPtr->property == Common::PROPERTY::GATE_DRIVER_TYPE ||
                         itemPtr->property == Common::PROPERTY::GATE_CONFIG_STATUS)
                {
                    ImGui::Text("%s: %d", itemPtr->label, itemPtr->value);
                }
                else if (itemPtr->property == Common::PROPERTY::INVERSE_TORQUE)
                {
                    bool inverseTorque = itemPtr->value != 0;
                    if (ImGui::Checkbox((std::string(itemPtr->label) + idSuffix).c_str(), &inverseTorque))
                    {
                        itemPtr->value = inverseTorque ? 1 : 0;
                    }
                }
                else
                {
                    ImGui::InputScalar((std::string(itemPtr->label) + idSuffix).c_str(),
                                       ImGuiDataType_S32,
                                       &itemPtr->value);
                }

                ImGui::SameLine();
                if (ImGui::Button("R##read", ImVec2(30, 0)))
                {
                    readConfigItem(*itemPtr, deviceAddress, serial, logs);
                }

                ImGui::SameLine();
                const bool writable = isPropertyWritable(itemPtr->property);
                if (!writable)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::Button("W##write", ImVec2(30, 0)))
                {
                    writeConfigItem(*itemPtr, deviceAddress, serial, logs);
                }
                if (!writable)
                {
                    ImGui::EndDisabled();
                }

                ImGui::PopID();
            }
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Read All"))
    {
        if (!serial.isConnected())
        {
            addLog(logs, "Read all failed: not connected.");
        }
        else
        {
            std::size_t successCount = 0;
            for (auto& item : configItems)
            {
                int32_t readValue = item.value;
                std::string error;
                if (serial.readProperty(deviceAddress, item.property, readValue, error))
                {
                    item.value = readValue;
                    ++successCount;
                }
                else
                {
                    addLog(logs, std::string("Read failed for ") + item.label + ": " + error);
                }
            }

            addLog(logs,
                   "Read all completed: " + std::to_string(successCount) + "/" + std::to_string(configItems.size())
                       + " fields updated.");
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Store"))
    {
        if (!serial.isConnected())
        {
            addLog(logs, "Store failed: not connected.");
        }
        else
        {
            std::string error;
            if (serial.sendFlashWriteCommand(deviceAddress, error))
            {
                addLog(logs, "Store command sent successfully.");
            }
            else
            {
                addLog(logs, std::string("Store failed: ") + error);
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Save Configuration"))
    {
        saveConfigToJson(configItems, "config.json", logs);
    }

    ImGui::SameLine();
    if (ImGui::Button("Load Configuration"))
    {
        loadConfigFromJson(configItems, "config.json", logs);
    }
}
