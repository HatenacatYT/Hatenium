#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"
#include "rendering/Hud.h"
#include "core/Utf.h"
#include "core/HudThemes.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace hat::gui {

void DrawHudPage(Application& app) {
    Settings& s = app.settings();
    KeyOverlaySettings& k = s.hud.keys;
    CpsSettings& c = s.hud.cps;

    int cols = ui::ColumnCountForWidth(330);
    if (!ui::BeginColumns("##hud_cols", cols)) return;

    ui::BeginCard("HUD Overlay");
    ui::NoteText("HUD elements are drawn by the screen overlay. Inputs are only polled for display - nothing is hooked, injected or stored.");
    ImGui::Dummy(ImVec2(0, S(2)));
    ui::Checkbox("Enable Overlay", &s.visuals.overlay.enable);
    ImGui::Dummy(ImVec2(0, S(4)));
    if (ui::Button("Edit Layout", ImVec2(ImGui::GetContentRegionAvail().x, 0), true)) app.StartLayoutEdit();
    ui::NoteText("Edit mode: drag the HUD elements on your screen. They snap to the edges and the centre. Ctrl + mouse wheel resizes the element under the cursor. Positions are remembered per monitor resolution (layout.json).");
    ui::EndCard();

    // ---------------------------------------------------------------- themes
    {
        HudThemes& th = app.themes();
        ui::BeginCard("HUD Theme");
        static std::vector<std::string> names;
        static std::vector<const char*> items;
        names.clear(); items.clear();
        for (int i = 0; i < th.Count(); ++i) names.push_back(th.At(i).name + (th.IsBuiltin(i) ? "" : " *"));
        for (auto& n : names) items.push_back(n.c_str());
        int cur = th.IndexOf(s.hud.theme);
        if (cur < 0) cur = 0;
        if (ui::Combo("Theme", &cur, items.data(), static_cast<int>(items.size()))) HudThemes::Apply(th.At(cur), s.hud);
        ui::NoteText("Sets background, text, accent colour and border width of the HUD.");
        ImGui::Dummy(ImVec2(0, S(2)));
        float bt = s.hud.borderThickness;
        if (ui::SliderFloat("Border Width", &bt, 0.f, 6.f, "%.1f px")) s.hud.borderThickness = bt;
        ui::ColorField("Background", s.hud.background);
        static char newName[48] = "";
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##themename", tr("New theme name"), newName, sizeof(newName));
        float bw = (ImGui::GetContentRegionAvail().x - S(8)) * 0.5f;
        if (ui::Button("Save as Theme", ImVec2(bw, 0), true) && newName[0] && th.IndexOf(newName) < 0) {
            HudTheme t = HudThemes::FromHud(s.hud, newName);
            th.AddOrReplaceUser(t); th.Save(); s.hud.theme = newName; newName[0] = 0;
        }
        ImGui::SameLine(0, S(8));
        if (ui::Button("Delete Theme", ImVec2(bw, 0)) && !th.IsBuiltin(cur)) { th.RemoveUser(cur); th.Save(); s.hud.theme = th.At(0).name; }
        if (ui::Button("Export Theme", ImVec2(bw, 0))) app.ExportTheme(cur);
        ImGui::SameLine(0, S(8));
        if (ui::Button("Import Theme", ImVec2(bw, 0))) app.ImportTheme();
        ui::EndCard();
    }

    ui::BeginCard("CPS Counter");
    ui::Checkbox("Enable", &c.enable);
    ui::Checkbox("Show Max CPS", &c.showMax);
    ui::SliderFloat("Scale", &c.scale, 0.5f, 4.f, "%.2fx");
    ui::SliderFloat("Position X", &c.posX, 0.f, 100.f, "%.1f%%");
    ui::SliderFloat("Position Y", &c.posY, 0.f, 100.f, "%.1f%%");
    ui::ColorField("Color", c.color);
    float co = c.opacity * 100.f;
    if (ui::SliderFloat("Opacity", &co, 10.f, 100.f, "%.0f%%")) c.opacity = co / 100.f;
    ImGui::Dummy(ImVec2(0, S(4)));
    if (ui::Button("Reset CPS", ImVec2(ImGui::GetContentRegionAvail().x, 0))) RequestCpsReset();
    ui::EndCard();

    ui::BeginCard("Key Overlay");
    ui::Checkbox("Enable", &k.enable);
    ui::Checkbox("Show Labels", &k.showLabels);
    ui::SliderFloat("Scale", &k.scale, 0.5f, 3.f, "%.2fx");
    ui::SliderFloat("Position X", &k.posX, 0.f, 100.f, "%.1f%%");
    ui::SliderFloat("Position Y", &k.posY, 0.f, 100.f, "%.1f%%");
    float ko = k.opacity * 100.f;
    if (ui::SliderFloat("Opacity", &ko, 10.f, 100.f, "%.0f%%")) k.opacity = ko / 100.f;
    ui::ColorField("Active Color", k.activeColor);
    ui::ColorField("Inactive Color", k.inactiveColor);
    ui::ColorField("Text Color", k.textColor);
    ui::EndCard();

    ui::BeginCard("Keys");
    ui::NoteText("Choose which keys are shown.");
    ImGui::Dummy(ImVec2(0, S(2)));
    ui::Checkbox("W", &k.w); ui::Checkbox("A", &k.a); ui::Checkbox("S", &k.s); ui::Checkbox("D", &k.d);
    ui::Checkbox("Space", &k.space); ui::Checkbox("Shift", &k.shift); ui::Checkbox("Ctrl", &k.ctrl);
    ui::Checkbox("Mouse Left", &k.lmb); ui::Checkbox("Mouse Right", &k.rmb);
    ui::EndCard();

    ui::BeginCard("Watermark Image");
    {
        WatermarkSettings& w = s.hud.watermark;
        ui::Checkbox("Enable", &w.enable);
        static char path[512] = "";
        static std::string lastPath;
        if (lastPath != w.path) { lastPath = w.path; std::strncpy(path, w.path.c_str(), sizeof(path) - 1); path[sizeof(path) - 1] = 0; }
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputTextWithHint("##wmpath", tr("Path to a PNG / JPEG image"), path, sizeof(path))) { w.path = path; lastPath = w.path; }
        if (ui::Button("Browse Image", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            std::wstring f = app.window().ImageDialog();
            if (!f.empty()) { w.path = Utf8FromWide(f); w.enable = true; }
        }
        ui::SliderFloat("Scale", &w.scale, 0.05f, 3.f, "%.2fx");
        float o = w.opacity * 100.f;
        if (ui::SliderFloat("Opacity", &o, 0.f, 100.f, "%.0f%%")) w.opacity = o / 100.f;
        ui::SliderFloat("Position X", &w.posX, 0.f, 100.f, "%.1f%%");
        ui::SliderFloat("Position Y", &w.posY, 0.f, 100.f, "%.1f%%");
    }
    ui::EndCard();

    ui::BeginCard("Timer HUD");
    ui::Checkbox("Enable", &s.hud.timer.enable);
    ui::NoteText("Shows the timer / stopwatch from the Tools tab on the overlay.");
    ui::SliderFloat("Scale", &s.hud.timer.scale, 0.5f, 4.f, "%.2fx");
    ui::SliderFloat("Position X", &s.hud.timer.posX, 0.f, 100.f, "%.1f%%");
    ui::SliderFloat("Position Y", &s.hud.timer.posY, 0.f, 100.f, "%.1f%%");
    ui::EndCard();

    ui::BeginCard("Presets");
    ui::NoteText("Share your crosshair, FOV circle, trail, particles and HUD as one .json file.");
    ImGui::Dummy(ImVec2(0, S(2)));
    {
        float pw = (ImGui::GetContentRegionAvail().x - S(8)) * 0.5f;
        if (ui::Button("Export Preset", ImVec2(pw, 0), true)) app.ExportPreset();
        ImGui::SameLine(0, S(8));
        if (ui::Button("Import Preset", ImVec2(pw, 0))) app.ImportPreset();
    }
    ui::EndCard();

    ui::BeginCard("Optional Keys");
    ui::Checkbox("R (Reload)", &k.r); ui::Checkbox("E", &k.e); ui::Checkbox("Q", &k.q); ui::Checkbox("F", &k.f);
    ui::Checkbox("Mouse Side 1", &k.side1); ui::Checkbox("Mouse Side 2", &k.side2);
    ui::EndCard();

    ui::EndColumns();
}

} // namespace hat::gui
