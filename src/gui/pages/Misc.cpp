#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"

namespace hat::gui {

void DrawMiscPage(Application& app) {
    MiscSettings& m = app.settings().misc;

    int cols = ui::ColumnCountForWidth(330);
    if (!ui::BeginColumns("##misc_cols", cols)) return;

    ImGui::TableNextColumn();
    ui::BeginCard("Interface");
    ui::Checkbox("Watermark", &m.watermark);
    ui::Checkbox("Animated Accent", &m.animatedAccent);
    ui::Checkbox("FPS Counter", &m.fpsCounter);
    ui::Checkbox("UI Animation", &m.uiAnimation);
    float po = m.panelOpacity * 100.f;
    if (ui::SliderFloat("Panel Opacity", &po, 30.f, 100.f, "%.0f%%")) m.panelOpacity = po / 100.f;
    ui::EndCard();

    ImGui::TableNextColumn();
    ui::BeginCard("Notification System");
    ui::Checkbox("Notification System", &m.notifications);
    ImGui::Dummy(ImVec2(0, S(4)));
    if (ui::Button("Test Notification", ImVec2(ImGui::GetContentRegionAvail().x, 0), true))
        ui::Notify(tr("This is a test notification"), 0);
    ui::EndCard();

    ui::EndColumns();
}

} // namespace hat::gui
