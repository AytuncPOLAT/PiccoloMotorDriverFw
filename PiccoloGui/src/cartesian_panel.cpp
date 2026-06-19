#include "cartesian_panel.hpp"
#include "imgui.h"
#include <cmath>
#include <cstdio>

void drawCartesianPanel(std::function<void(float, float, float)> updateCallback)
{
    ImGui::TextUnformatted("Cartesian Panel");
    ImGui::Separator();

    // (Sliders for X/Y/Z will be shown below after declaring persistent state)

    const float xMin = -1000.0f;
    const float xMax = 1000.0f;
    const float yMin = -1000.0f;
    const float yMax = 1000.0f;

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
    if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background and border
    ImU32 bgCol = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 borderCol = ImGui::GetColorU32(ImGuiCol_Border);
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), bgCol);
    drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), borderCol);

    // Invisible button for mouse interaction
    ImGui::InvisibleButton("cart_canvas", canvasSize);
    const bool isHovered = ImGui::IsItemHovered();

    // Persistent state: 3D sphere position
    static float worldX = 0.0f;
    static float worldY = 0.0f;
    static float worldZ = 0.0f;
    static bool grabbed = false;
    static ImVec2 grabOffset(0, 0);

    // Sliders for X/Y/Z
    ImGui::PushItemWidth(-1);
    ImGui::SliderFloat("X", &worldX, -1000.0f, 1000.0f, "%.0f");
    ImGui::SliderFloat("Y", &worldY, -1000.0f, 1000.0f, "%.0f");
    ImGui::SliderFloat("Z", &worldZ, -1000.0f, 1000.0f, "%.0f");
    ImGui::PopItemWidth();
    // Camera state for 3D view
    static float camYaw = 0.6f;   // radians
    static float camPitch = -0.25f; // radians
    static float camDistance = 2500.0f;
    const float fov = 45.0f * 3.14159265f / 180.0f;

    ImVec2 mousePos = ImGui::GetIO().MousePos;

    // Helper math
    auto clamp01 = [](float v){ return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };

    struct Vec3 { float x,y,z; };
    auto add = [](const Vec3&a,const Vec3&b){ return Vec3{a.x+b.x,a.y+b.y,a.z+b.z}; };
    auto sub = [](const Vec3&a,const Vec3&b){ return Vec3{a.x-b.x,a.y-b.y,a.z-b.z}; };
    auto mul = [](const Vec3&a,float s){ return Vec3{a.x*s,a.y*s,a.z*s}; };
    auto dot = [](const Vec3&a,const Vec3&b){ return a.x*b.x + a.y*b.y + a.z*b.z; };
    auto cross = [](const Vec3&a,const Vec3&b){ return Vec3{a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; };
    auto norm = [&](const Vec3&a){ float l = std::sqrt(dot(a,a)); if (l==0) return a; return mul(a, 1.0f / l); };

    // Camera math: compute camera basis and projection
    Vec3 target = Vec3{0.0f, 0.0f, 0.0f};
    Vec3 camDir = Vec3{std::cos(camPitch)*std::sin(camYaw), std::sin(camPitch), std::cos(camPitch)*std::cos(camYaw)};
    Vec3 camPos = sub(target, mul(camDir, camDistance));
    Vec3 worldUp = Vec3{0.0f, 1.0f, 0.0f};
    Vec3 camRight = norm(cross(camDir, worldUp));
    Vec3 camUp = norm(cross(camRight, camDir));

    float aspect = canvasSize.x / canvasSize.y;
    float tanFov = std::tan(fov * 0.5f);

    auto projectPoint = [&](const Vec3& p)->ImVec2 {
        // Transform point to camera space
        Vec3 v = sub(p, camPos);
        // Camera basis: right, up, forward (camDir)
        float cx = dot(v, camRight);
        float cy = dot(v, camUp);
        float cz = dot(v, camDir);
        if (cz <= 1e-6f) cz = 1e-6f;
        // Perspective projection to NDC
        float ndcX = (cx / (cz * tanFov * aspect));
        float ndcY = (cy / (cz * tanFov));
        float sx = canvasPos.x + (ndcX * 0.5f + 0.5f) * canvasSize.x;
        float sy = canvasPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * canvasSize.y;
        return ImVec2(sx, sy);
    };

    auto screenToWorldRay = [&](ImVec2 sp)->std::pair<Vec3, Vec3> {
        float ndcX = ((sp.x - canvasPos.x) / canvasSize.x) * 2.0f - 1.0f;
        float ndcY = 1.0f - ((sp.y - canvasPos.y) / canvasSize.y) * 2.0f;
        // Ray in camera space
        Vec3 rayCam = Vec3{ndcX * tanFov * aspect, ndcY * tanFov, 1.0f};
        // Convert to world
        Vec3 dir = add(add(mul(camRight, rayCam.x), mul(camUp, rayCam.y)), mul(camDir, rayCam.z));
        dir = norm(dir);
        return { camPos, dir };
    };

    // Project current sphere center
    ImVec2 circlePos = projectPoint(Vec3{worldX, worldY, worldZ});
    // Sphere visual: radius scales with Z to give depth illusion.
    const float baseRadius = std::min(canvasSize.x, canvasSize.y) * 0.03f;
    const float zMin = -1000.0f;
    const float zMax = 1000.0f;
    const float zNorm = (worldZ - zMin) / (zMax - zMin); // 0..1
    const float radius = std::max(4.0f, baseRadius * (0.6f + zNorm * 1.4f));

    // Camera rotation with Shift+drag
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyShift && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
    {
        camYaw += io.MouseDelta.x * -0.005f; // invert to feel natural
        camPitch += io.MouseDelta.y * 0.005f;
        const float maxPitch = 3.14159265f * 0.5f - 0.01f;
        if (camPitch > maxPitch) camPitch = maxPitch;
        if (camPitch < -maxPitch) camPitch = -maxPitch;
        // don't start sphere drag when rotating
    }
    else
    {
        // Start drag if clicking the sphere (only when not rotating)
        if (ImGui::IsMouseClicked(0) && isHovered)
        {
            float dx = mousePos.x - circlePos.x;
            float dy = mousePos.y - circlePos.y;
            if (dx * dx + dy * dy <= radius * radius)
            {
                grabbed = true;
                // compute initial intersection offset
                auto [ro, rd] = screenToWorldRay(mousePos);
                Vec3 planeP = Vec3{worldX, worldY, worldZ};
                Vec3 planeN = camDir; // plane normal = camera forward
                float denom = dot(rd, planeN);
                if (std::fabs(denom) > 1e-6f)
                {
                    Vec3 diff = sub(planeP, ro);
                    float t = dot(diff, planeN) / denom;
                    Vec3 hit = add(ro, mul(rd, t));
                    grabOffset = ImVec2(hit.x - worldX, hit.y - worldY);
                }
                else
                {
                    grabOffset = ImVec2(0,0);
                }
            }
        }

        // While grabbed and mouse held, update sphere 3D position by ray-plane intersection
        if (grabbed && ImGui::IsMouseDown(0))
        {
            auto [ro, rd] = screenToWorldRay(mousePos);
            Vec3 planeP = Vec3{worldX, worldY, worldZ};
            Vec3 planeN = camDir;
            float denom = dot(rd, planeN);
            if (std::fabs(denom) > 1e-6f)
            {
                Vec3 diff = sub(planeP, ro);
                float t = dot(diff, planeN) / denom;
                Vec3 hit = add(ro, mul(rd, t));
                float nx = hit.x - grabOffset.x;
                float ny = hit.y - grabOffset.y;
                float nz = hit.z; // keep depth from plane intersection
                // clamp to allowed world bounds
                worldX = std::max(xMin, std::min(xMax, nx));
                worldY = std::max(yMin, std::min(yMax, ny));
                worldZ = std::max(zMin, std::min(zMax, nz));
            }
        }

        if (grabbed && ImGui::IsMouseReleased(0))
        {
            grabbed = false;
        }
    }

    // Draw 3D axes (X: red, Y: green, Z: blue)
    ImU32 axisColX = ImGui::GetColorU32(ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImU32 axisColY = ImGui::GetColorU32(ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
    ImU32 axisColZ = ImGui::GetColorU32(ImVec4(0.2f, 0.45f, 0.9f, 1.0f));
    ImVec2 axX1 = projectPoint(Vec3{xMin, 0.0f, 0.0f});
    ImVec2 axX2 = projectPoint(Vec3{xMax, 0.0f, 0.0f});
    ImVec2 axY1 = projectPoint(Vec3{0.0f, yMin, 0.0f});
    ImVec2 axY2 = projectPoint(Vec3{0.0f, yMax, 0.0f});
    ImVec2 axZ1 = projectPoint(Vec3{0.0f, 0.0f, -1000.0f});
    ImVec2 axZ2 = projectPoint(Vec3{0.0f, 0.0f, 1000.0f});
    drawList->AddLine(axX1, axX2, axisColX, 2.0f);
    drawList->AddLine(axY1, axY2, axisColY, 2.0f);
    drawList->AddLine(axZ1, axZ2, axisColZ, 2.0f);

    // Draw sphere: base shading depends on Z (closer = brighter)
    circlePos = projectPoint(Vec3{worldX, worldY, worldZ});
    float baseBrightness = 0.35f + 0.6f * zNorm; // 0.35..0.95
    ImVec4 baseCol = ImVec4(0.15f * baseBrightness, 0.45f * baseBrightness, 0.8f * baseBrightness, 1.0f);
    ImU32 circleFill = ImGui::GetColorU32(baseCol);
    ImU32 circleOutline = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    drawList->AddCircleFilled(circlePos, radius, circleFill);
    drawList->AddCircle(circlePos, radius, circleOutline, 32, 2.0f);

    // Simple specular highlight (offset toward top-left light)
    ImVec2 lightDir = ImVec2(-0.35f, -0.35f);
    ImVec2 highlightPos = ImVec2(circlePos.x + lightDir.x * radius * 0.4f, circlePos.y + lightDir.y * radius * 0.4f);
    ImU32 highlightCol = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.9f * (0.4f + zNorm * 0.6f)));
    drawList->AddCircleFilled(highlightPos, radius * 0.28f, highlightCol);

    // Display coordinates and Z in corner
    char buf[96];
    std::snprintf(buf, sizeof(buf), "(%.0f, %.0f, %.0f)", worldX, worldY, worldZ);
    drawList->AddText(ImVec2(canvasPos.x + 6, canvasPos.y + 6), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), buf);

    // Call update callback each frame
    if (updateCallback)
    {
        updateCallback(worldX, worldY, worldZ);
    }
}
