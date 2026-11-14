#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"
#include <string>

namespace hat::gui {

namespace {
void NotifyToggle(const char* name, bool on) {
    ui::Notify(std::string(tr(name)) + " " + tr(on ? "enabled" : "disabled"), on ? 1 : 0);
}
}

void DrawCombatPage(Application& app) {
    CombatSettings& c = app.settings().combat;
    static const char* targets[] = {"Closest", "Head", "Chest", "Body"};

    int cols = ui::ColumnCountForWidth(330);
    if (!ui::BeginColumns("##combat_cols", cols)) return;

    ImGui::TableNextColumn();
    ui::BeginCard("Aimbot");
    if (ui::Checkbox("Enable", &c.aimbot.enable)) NotifyToggle("Aimbot", c.aimbot.enable);
    ui::SliderFloat("FOV", &c.aimbot.fov, 1.f, 180.f, "%.0f");
    ui::SliderFloat("Smoothness", &c.aimbot.smoothness, 1.f, 30.f, "%.1f");
    ui::Combo("Target Selection", &c.aimbot.target, targets, 4);
    ui::Keybind("Keybind", &c.aimbot.key);
    ui::EndCard();

    ui::BeginCard("Recoil Control");
    if (ui::Checkbox("Enable", &c.recoil.enable)) NotifyToggle("Recoil Control", c.recoil.enable);
    ui::SliderFloat("Horizontal", &c.recoil.horizontal, 0.f, 100.f, "%.0f%%");
    ui::SliderFloat("Vertical", &c.recoil.vertical, 0.f, 100.f, "%.0f%%");
    ui::EndCard();

    ImGui::TableNextColumn();
    ui::BeginCard("Triggerbot");
    if (ui::Checkbox("Enable", &c.triggerbot.enable)) NotifyToggle("Triggerbot", c.triggerbot.enable);
    ui::Keybind("Keybind", &c.triggerbot.key);
    ui::SliderFloat("Delay", &c.triggerbot.delayMs, 0.f, 500.f, "%.0f ms");
    ui::SliderFloat("Hit Chance", &c.triggerbot.hitChance, 0.f, 100.f, "%.0f%%");
    ui::EndCard();

    ui::BeginCard("Other Combat");
    ui::Checkbox("No Recoil", &c.other.noRecoil);
    ui::Checkbox("No Spread", &c.other.noSpread);
    ui::Checkbox("Instant Reload", &c.other.instantReload);
    ui::EndCard();

    ui::EndColumns();
}

} // namespace hat::gui
