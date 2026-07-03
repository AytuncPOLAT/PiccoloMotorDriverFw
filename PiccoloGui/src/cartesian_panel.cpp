#include "cartesian_panel.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

void drawCartesianPanel(std::function<void(float, float, float, float, float)> updateCallback)
{
    ImGui::TextUnformatted("Cartesian Panel");
    ImGui::Separator();

    static float worldX = 0.0f;
    static float worldY = 0.0f;
    static float worldZ = 0.0f;
    static float linkLength1 = 100.0f;
    static float linkLength2 = 100.0f;
    static bool elbowPositive = true;

    const float reach = linkLength1 + linkLength2;
    const float minReach = std::fabs(linkLength1 - linkLength2);

    ImGui::PushItemWidth(-1);
    ImGui::InputFloat("Link 1 length", &linkLength1, 1.0f, 10.0f, "%.1f");
    ImGui::InputFloat("Link 2 length", &linkLength2, 1.0f, 10.0f, "%.1f");
    ImGui::Checkbox("Elbow positive", &elbowPositive);
    ImGui::Spacing();

    if (linkLength1 < 1.0f) linkLength1 = 1.0f;
    if (linkLength2 < 1.0f) linkLength2 = 1.0f;

    ImGui::SliderFloat("X", &worldX, -reach, reach, "%.0f");
    ImGui::SliderFloat("Y", &worldY, -reach, reach, "%.0f");
    ImGui::SliderFloat("Z", &worldZ, -1000.0f, 1000.0f, "%.0f");
    ImGui::PopItemWidth();

    float dist = std::sqrt(worldX * worldX + worldY * worldY);
    if (dist > reach)
    {
        float scale = reach / dist;
        worldX *= scale;
        worldY *= scale;
        dist = reach;
    }
    else if (dist < minReach && dist > 1e-6f)
    {
        float scale = minReach / dist;
        worldX *= scale;
        worldY *= scale;
        dist = minReach;
    }
    else if (dist <= 1e-6f)
    {
        worldX = minReach;
        worldY = 0.0f;
        dist = minReach;
    }

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 150.0f) canvasSize.x = 150.0f;
    if (canvasSize.y < 150.0f) canvasSize.y = 150.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 bgCol = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 borderCol = ImGui::GetColorU32(ImGuiCol_Border);
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), bgCol);
    drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), borderCol);

    ImGui::InvisibleButton("cart_canvas", canvasSize);
    bool isHovered = ImGui::IsItemHovered();
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    static bool grabbed = false;
    static ImVec2 grabOffset(0, 0);

    float displayRange = std::max(reach * 1.15f, 100.0f);
    float scale = std::min((canvasSize.x - 20.0f) / (displayRange * 2.0f), (canvasSize.y - 20.0f) / (displayRange * 2.0f));
    ImVec2 center = ImVec2(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);

    auto worldToScreen = [&](float x, float y) {
        return ImVec2(center.x + x * scale, center.y - y * scale);
    };

    ImU32 axisColor = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImU32 link1Color = ImGui::GetColorU32(ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImU32 link2Color = ImGui::GetColorU32(ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImU32 effColor = ImGui::GetColorU32(ImVec4(0.2f, 0.45f, 0.9f, 1.0f));
    ImU32 reachColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.15f));

    ImVec2 origin = worldToScreen(0.0f, 0.0f);
    drawList->AddLine(ImVec2(center.x - displayRange * scale, center.y), ImVec2(center.x + displayRange * scale, center.y), axisColor, 2.0f);
    drawList->AddLine(ImVec2(center.x, center.y - displayRange * scale), ImVec2(center.x, center.y + displayRange * scale), axisColor, 2.0f);
    drawList->AddCircle(origin, reach * scale, reachColor, 64, 1.0f);

    float cosAngle2 = (dist * dist - linkLength1 * linkLength1 - linkLength2 * linkLength2) / (2.0f * linkLength1 * linkLength2);
    cosAngle2 = std::clamp(cosAngle2, -1.0f, 1.0f);
    float angle2 = std::acos(cosAngle2);
    if (!elbowPositive)
    {
        angle2 = -angle2;
    }
    float k1 = linkLength1 + linkLength2 * cosAngle2;
    float k2 = linkLength2 * std::sin(angle2);
    float angle1 = std::atan2(worldY, worldX) - std::atan2(k2, k1);

    ImVec2 joint1 = worldToScreen(linkLength1 * std::cos(angle1), linkLength1 * std::sin(angle1));
    ImVec2 endEffector = worldToScreen(worldX, worldY);

    drawList->AddLine(origin, joint1, link1Color, 4.0f);
    drawList->AddLine(joint1, endEffector, link2Color, 4.0f);
    drawList->AddCircleFilled(origin, 6.0f, axisColor);
    drawList->AddCircleFilled(joint1, 6.0f, link1Color);
    drawList->AddCircleFilled(endEffector, 8.0f, effColor);

    if (isHovered && ImGui::IsMouseClicked(0))
    {
        float dx = mousePos.x - endEffector.x;
        float dy = mousePos.y - endEffector.y;
        if (dx * dx + dy * dy <= 10.0f * 10.0f)
        {
            grabbed = true;
            grabOffset = ImVec2(endEffector.x - mousePos.x, endEffector.y - mousePos.y);
        }
    }

    if (grabbed && ImGui::IsMouseDown(0))
    {
        ImVec2 pos = ImVec2(mousePos.x + grabOffset.x, mousePos.y + grabOffset.y);
        float wx = (pos.x - center.x) / scale;
        float wy = (center.y - pos.y) / scale;
        float d = std::sqrt(wx * wx + wy * wy);
        if (d > reach)
        {
            float s = reach / d;
            wx *= s;
            wy *= s;
        }
        else if (d < minReach && d > 1e-6f)
        {
            float s = minReach / d;
            wx *= s;
            wy *= s;
        }
        worldX = wx;
        worldY = wy;
    }

    if (grabbed && !ImGui::IsMouseDown(0))
    {
        grabbed = false;
    }

    char infoBuf[128];
    std::snprintf(infoBuf, sizeof(infoBuf), "θ1=%.1f°  θ2=%.1f°  dist=%.0f", angle1 * 180.0f / 3.14159265f, angle2 * 180.0f / 3.14159265f, dist);
    drawList->AddText(ImVec2(canvasPos.x + 8, canvasPos.y + canvasSize.y - 20), ImGui::GetColorU32(ImVec4(1,1,1,1)), infoBuf);

    if (updateCallback)
    {
        updateCallback(worldX, worldY, worldZ, angle1, angle2);
    }
}
