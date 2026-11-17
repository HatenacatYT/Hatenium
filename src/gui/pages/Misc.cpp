#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"

namespace hat::gui {

void DrawMiscPage(Application& app) {
    MiscSettings& m = app.settings().misc;

    int cols = ui::ColumnCountForWidth(330);
    if (!ui::BeginColumns("##misc_cols", cols)) return;

    ui::BeginCard("Interface");
    ui::Checkbox("Watermark", &m.watermark);
    ui::Checkbox("Animated Accent", &m.animatedAccent);
    ui::Checkbox("FPS Counter", &m.fpsCounter);
    ui::Checkbox("UI Animation", &m.uiAnimation);
    float po = m.panelOpacity * 100.f;
    if (ui::SliderFloat("Panel Opacity", &po, 30.f, 100.f, "%.0f%%")) m.panelOpacity = po / 100.f;
    ui::EndCard();

    ui::BeginCard("Power");
    ui::Checkbox("Prevent Sleep", &m.preventSleep);
    ui::NoteText("Keeps Windows and the display awake while Hatenium runs. No fake input is ever sent to any game.");
    ui::EndCard();

    ui::BeginCard("Notification System");
    ui::Checkbox("Notification System", &m.notifications);
    ImGui::Dummy(ImVec2(0, S(4)));
    if (ui::Button("Test Notification", ImVec2(ImGui::GetContentRegionAvail().x, 0), true))
        ui::Notify(tr("This is a test notification"), 0);
    ui::EndCard();

    ui::EndColumns();
}

} // namespace hat::gui
