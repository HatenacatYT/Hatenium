#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/AnimationManager.h"
#include "rendering/PreviewRenderer.h"

namespace hat::gui {

void DrawMovementPage(Application& app) {
    MovementSettings& m = app.settings().movement;
    static const char* presets[] = {"Default", "Smooth", "Aggressive", "Custom"};

    int cols = ui::ColumnCountForWidth(330);
    if (!ui::BeginColumns("##move_cols", cols)) return;

    ui::BeginCard("Movement");
    ui::Checkbox("Bunny Hop", &m.bunnyHop);
    ui::Checkbox("Auto Strafe", &m.autoStrafe);
    ui::Checkbox("Speed Indicator", &m.speedIndicator);
    ui::Combo("Movement Preset", &m.preset, presets, 4);
    ui::EndCard();

    if (m.speedIndicator) {
        ui::BeginCard("Speed Indicator");
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x, h = S(104);
        ImGui::Dummy(ImVec2(w, h));
        DrawSpeedPreview(ImGui::GetWindowDrawList(), p, ImVec2(p.x + w, p.y + h), AnimationManager::Get().Time());
        ui::EndCard();
    }

    ui::EndColumns();
}

} // namespace hat::gui
