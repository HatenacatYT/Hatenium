#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/AnimationManager.h"
#include "core/Localization.h"
#include "rendering/PreviewRenderer.h"
#include "rendering/OverlayWindow.h"
#include <string>
#include <vector>
#include <algorithm>

namespace hat::gui {

void DrawVisualsPage(Application& app) {
    VisualSettings& v = app.settings().visuals;
    static const char* styles[] = {"Cross", "Dot", "Circle", "Cross + Dot"};

    // ---- preview canvas (full width, always 16:9-ish, capped in height) ----
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = std::min(w * 0.5625f, S(300));
    ImGui::Dummy(ImVec2(w, h));
    DrawVisualPreview(ImGui::GetWindowDrawList(), p, ImVec2(p.x + w, p.y + h), v, AnimationManager::Get().Time());
    ImGui::Dummy(ImVec2(0, S(2)));

    int cols = ui::ColumnCountForWidth(330);
    if (!ui::BeginColumns("##vis_cols", cols)) return;

    ui::BeginCard("FOV Circle");
    ui::Checkbox("Enable", &v.fovCircle.enable);
    ui::SliderFloat("Radius", &v.fovCircle.radius, 10.f, 300.f, "%.0f");
    ui::SliderFloat("Thickness", &v.fovCircle.thickness, 0.5f, 6.f, "%.1f");
    float op = v.fovCircle.opacity * 100.f;
    if (ui::SliderFloat("Opacity", &op, 0.f, 100.f, "%.0f%%")) v.fovCircle.opacity = op / 100.f;
    ui::ColorField("Color", v.fovCircle.color);
    ui::Checkbox("Filled Circle", &v.fovCircle.filled);
    ui::Checkbox("Outline", &v.fovCircle.outline);
    ui::SliderFloat("Center X", &v.fovCircle.centerX, -100.f, 100.f, "%.0f");
    ui::SliderFloat("Center Y", &v.fovCircle.centerY, -100.f, 100.f, "%.0f");
    ui::EndCard();

    ui::BeginCard("Screen Overlay");
    ui::NoteText("Draws on top of your screen (click-through). Only pixels are drawn - Hatenium never touches other programs.");
    ImGui::Dummy(ImVec2(0, S(2)));
    if (ui::Checkbox("Enable Overlay", &v.overlay.enable))
        ui::Notify(std::string(tr("Screen Overlay")) + " " + tr(v.overlay.enable ? "enabled" : "disabled"), v.overlay.enable ? 1 : 0);
    {
        static std::vector<std::string> names;
        static std::vector<const char*> items;
        int mc = std::min(OverlayWindow::MonitorCount(), 6);
        if (static_cast<int>(names.size()) != mc + 1) {
            names.clear(); items.clear();
            for (int i = 0; i < mc; ++i) names.push_back(i == 0 ? "Primary Monitor" : "Monitor " + std::to_string(i + 1));
            names.push_back("All Monitors");
            for (auto& n : names) items.push_back(n.c_str());
        }
        int idx = (v.overlay.monitor == 100) ? mc : std::min(v.overlay.monitor, mc - 1);
        if (ui::Combo("Overlay Monitor", &idx, items.data(), mc + 1)) v.overlay.monitor = (idx == mc) ? 100 : idx;
    }
    ui::EndCard();

    ui::BeginCard("Crosshair");
    ui::Checkbox("Enable", &v.crosshair.enable);
    ui::Combo("Style", &v.crosshair.style, styles, 4);
    ui::SliderFloat("Size", &v.crosshair.size, 2.f, 40.f, "%.0f");
    ui::SliderFloat("Gap", &v.crosshair.gap, 0.f, 20.f, "%.0f");
    ui::SliderFloat("Thickness", &v.crosshair.thickness, 0.5f, 6.f, "%.1f");
    ui::ColorField("Color", v.crosshair.color);
    float cop = v.crosshair.opacity * 100.f;
    if (ui::SliderFloat("Opacity", &cop, 0.f, 100.f, "%.0f%%")) v.crosshair.opacity = cop / 100.f;
    ui::EndCard();

    ui::BeginCard("Crosshair Animation");
    ui::Checkbox("Spread on Shoot", &v.crosshair.spreadOnShoot);
    ui::Checkbox("Shrink on Move", &v.crosshair.shrinkOnMove);
    ui::SliderFloat("Amount", &v.crosshair.animAmount, 0.f, 40.f, "%.0f px");
    ui::SliderFloat("Anim Speed", &v.crosshair.animSpeed, 1.f, 40.f, "%.0f");
    static const char* eases[] = {"Linear", "Ease Out"};
    ui::Combo("Easing", &v.crosshair.easing, eases, 2);
    ui::NoteText("Shows on the screen overlay (left mouse button / W A S D are polled, not recorded).");
    ui::EndCard();

    ui::BeginCard("ESP Settings");
    ui::NoteText("Settings only - nothing is drawn or detected.");
    ImGui::Dummy(ImVec2(0, S(2)));
    ui::Checkbox("Box ESP", &v.esp.box);
    ui::Checkbox("Name ESP", &v.esp.name);
    ui::Checkbox("Distance ESP", &v.esp.distance);
    ui::Checkbox("Snaplines", &v.esp.snaplines);
    ui::EndCard();

    {
        static const char* trailStyles[] = {"Line", "Dots", "Comet"};
        static const char* trailColors[] = {"Solid", "Gradient", "Rainbow"};
        MouseTrailSettings& t = v.trail;
        ui::BeginCard("Mouse Trail");
        ui::Checkbox("Enable", &t.enable);
        ui::Combo("Style", &t.style, trailStyles, 3);
        ui::Combo("Color Mode", &t.colorMode, trailColors, 3);
        if (t.colorMode != 2) ui::ColorField("Color", t.color);
        if (t.colorMode == 1) ui::ColorField("Color 2", t.color2);
        if (t.colorMode == 2) ui::SliderFloat("Rainbow Speed", &t.rainbowSpeed, 0.1f, 5.f, "%.1fx");
        ui::SliderFloat("Thickness", &t.thickness, 1.f, 30.f, "%.0f px");
        ui::SliderFloat("Lifetime", &t.lifetimeMs, 50.f, 2000.f, "%.0f ms");
        float to = t.opacity * 100.f;
        if (ui::SliderFloat("Opacity", &to, 0.f, 100.f, "%.0f%%")) t.opacity = to / 100.f;
        ui::Checkbox("Taper", &t.taper);
        ui::Checkbox("Glow", &t.glow);
        ui::EndCard();

        static const char* shapes[] = {"Circle", "Square", "Star", "Ring"};
        static const char* pColors[] = {"Solid", "Gradient", "Rainbow", "Random"};
        ClickParticleSettings& pc = v.particles;
        ui::BeginCard("Click Particles");
        ui::Checkbox("Enable", &pc.enable);
        ui::Checkbox("On Left Click", &pc.onLeft);
        ui::Checkbox("On Right Click", &pc.onRight);
        ui::SliderInt("Count", &pc.count, 1, 80, "%d");
        ui::Combo("Shape", &pc.shape, shapes, 4);
        ui::Combo("Color Mode", &pc.colorMode, pColors, 4);
        if (pc.colorMode <= 1) ui::ColorField("Color", pc.color);
        if (pc.colorMode == 1) ui::ColorField("Color 2", pc.color2);
        ui::SliderFloat("Size", &pc.size, 1.f, 40.f, "%.0f px");
        ui::SliderFloat("Speed", &pc.speed, 20.f, 1200.f, "%.0f");
        ui::SliderFloat("Lifetime", &pc.lifetimeMs, 100.f, 3000.f, "%.0f ms");
        ui::SliderFloat("Gravity", &pc.gravity, -1000.f, 2000.f, "%.0f");
        float po = pc.opacity * 100.f;
        if (ui::SliderFloat("Opacity", &po, 0.f, 100.f, "%.0f%%")) pc.opacity = po / 100.f;
        ui::EndCard();
    }

    ui::EndColumns();
}

} // namespace hat::gui
