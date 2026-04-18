#include "log_panel.hpp"
#include "imgui.h"

void drawLogPanel(std::vector<std::string>& logs, bool& autoScrollLogs)
{
    ImGui::TextUnformatted("Communication Log");
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScrollLogs);
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        logs.clear();
    }
    ImGui::Separator();

    ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const std::string& line : logs)
    {
        ImGui::TextUnformatted(line.c_str());
    }
    if (autoScrollLogs && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 30.0f)
    {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}
