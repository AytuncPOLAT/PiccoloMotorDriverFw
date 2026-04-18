#include "configuration_panel.hpp"
#include "imgui.h"
#include "log_utils.hpp"
#include <map>
#include <string>

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
                ImGui::SetNextItemWidth(220.0f);
                ImGui::InputScalar((std::string(itemPtr->label) + idSuffix).c_str(),
                                   ImGuiDataType_S32,
                                   &itemPtr->value);

                ImGui::SameLine();
                if (ImGui::Button("Read"))
                {
                    readConfigItem(*itemPtr, deviceAddress, serial, logs);
                }

                ImGui::SameLine();
                const bool writable = isPropertyWritable(itemPtr->property);
                if (!writable)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::Button("Write"))
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
    if (ImGui::Button("Save Preset"))
    {
        addLog(logs, "Save preset clicked.");
    }
}
