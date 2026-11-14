#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/AnimationManager.h"
#include "core/Localization.h"
#include "rendering/PreviewRenderer.h"
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

    ImGui::TableNextColumn();
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

    ImGui::TableNextColumn();
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

    ui::BeginCard("ESP Settings");
    ui::NoteText("Settings only - nothing is drawn or detected.");
    ImGui::Dummy(ImVec2(0, S(2)));
    ui::Checkbox("Box ESP", &v.esp.box);
    ui::Checkbox("Name ESP", &v.esp.name);
    ui::Checkbox("Distance ESP", &v.esp.distance);
    ui::Checkbox("Snaplines", &v.esp.snaplines);
    ui::EndCard();

    ui::EndColumns();
}

} // namespace hat::gui
