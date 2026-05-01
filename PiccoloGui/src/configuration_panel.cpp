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
        if (serial.writeProperty(deviceAddress, item.property, item.value, error))
        {
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
    for (int catIdx = 0; catIdx < 3; ++catIdx)
    {
        ConfigCategory cat = static_cast<ConfigCategory>(catIdx);
        auto it = itemsByCategory.find(cat);
        if (it == itemsByCategory.end())
            continue;

        if (ImGui::CollapsingHeader(toCategoryLabel(cat), ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto itemPtr : it->second)
            {
                const int propertyId = static_cast<int>(itemPtr->property);
                const std::string idSuffix = "##prop_" + std::to_string(propertyId);

                ImGui::PushID(propertyId);
                ImGui::SetNextItemWidth(150.0f);
                ImGui::InputScalar((std::string(itemPtr->label) + idSuffix).c_str(),
                                   ImGuiDataType_S32,
                                   &itemPtr->value);

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

    if (ImGui::Button("Apply All Settings"))
    {
        if (!serial.isConnected())
        {
            addLog(logs, "Apply settings failed: not connected.");
        }
        else
        {
            std::size_t successCount = 0;
            for (const auto& item : configItems)
            {
                if (!isPropertyWritable(item.property))
                {
                    continue;
                }

                std::string error;
                if (serial.writeProperty(deviceAddress, item.property, item.value, error))
                {
                    ++successCount;
                }
                else
                {
                    addLog(logs, std::string("Write failed for ") + item.label + ": " + error);
                }
            }

            addLog(logs, "Apply settings completed: " + std::to_string(successCount) + " values written.");
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
