#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"
#include "core/Profiles.h"
#include <algorithm>
#include <string>

namespace hat::gui {

void DrawSettingsPage(Application& app) {
    Settings& s = app.settings();
    static const char* themes[] = {"Cyan", "Purple", "Red", "Green", "White", "Custom"};
    static const char* shapes[] = {"Square", "Slight Rounded", "Rounded", "Pill"};
    static const char* windows[] = {"Sharp", "Soft Rounded", "Rounded", "Extra Rounded"};
    static const char* languages[] = {"English", "日本語"};

    int cols = ui::ColumnCountForWidth(360);
    if (!ui::BeginColumns("##set_cols", cols)) return;

    // ---------------------------------------------------------- column 1
    ui::BeginCard("Appearance");
    ui::Combo("Theme Color", &s.theme, themes, 6);
    if (s.theme == 5) ui::ColorField("Custom Color", s.customColor);
    ui::Combo("Button Shape", &s.buttonShape, shapes, 4);
    ui::Combo("Window Appearance", &s.windowAppearance, windows, 4);
    {   // UI scale is committed on release (it rebuilds the font atlas)
        static float edit = 0.f; static bool editing = false;
        if (!editing) edit = s.uiScale;
        ui::SliderFloat("UI Scale", &edit, 0.75f, 2.0f, "%.2fx");
        editing = ImGui::IsItemActive();
        if (ImGui::IsItemDeactivated()) { s.uiScale = std::min(std::max(edit, 0.75f), 2.0f); editing = false; }
    }
    ui::EndCard();

    ui::BeginCard("Window");
    {
        int maxW = 0, maxH = 0;
        app.MaxLogicalWindowSize(maxW, maxH);
        static int ew = 0, eh = 0; static bool editW = false, editH = false;
        if (!editW) ew = s.windowW > 0 ? s.windowW : std::min(980, maxW);
        if (!editH) eh = s.windowH > 0 ? s.windowH : std::min(640, maxH);
        ui::SliderInt("Window Width", &ew, 720, std::max(maxW, 721), "%d px");
        editW = ImGui::IsItemActive();
        if (ImGui::IsItemDeactivated()) { editW = false; app.ResizeMainWindow(ew, eh); }
        ui::SliderInt("Window Height", &eh, 460, std::max(maxH, 461), "%d px");
        editH = ImGui::IsItemActive();
        if (ImGui::IsItemDeactivated()) { editH = false; app.ResizeMainWindow(ew, eh); }
    }
    ui::Checkbox("Always on Top", &s.alwaysOnTop);
    ui::Checkbox("Remember Position", &s.rememberPosition);
    ui::Checkbox("Remember Size", &s.rememberSize);
    ui::EndCard();

    // ---------------------------------------------------------- column 2
    ui::BeginCard("Application");
    ui::Checkbox("Startup Animation", &s.startupAnimation);
    ui::SliderFloat("Animation Speed", &s.animationSpeed, 0.25f, 3.f, "%.2fx");
    ui::Combo("Language", &s.language, languages, 2);
    ImGui::Dummy(ImVec2(0, S(4)));
    float bw = (ImGui::GetContentRegionAvail().x - S(8)) * 0.5f;
    if (ui::Button("Reset Settings", ImVec2(bw, 0))) ImGui::OpenPopup("##confirm_reset");
    ImGui::SameLine(0, S(8));
    if (ui::Button("Reopen Setup", ImVec2(bw, 0))) app.ReopenSetup();

    ImVec2 c = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(c, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, S(18, 16));
    if (ImGui::BeginPopupModal("##confirm_reset", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(tr("Reset everything to defaults?"));
        ImGui::Dummy(ImVec2(0, S(6)));
        if (ui::Button("Confirm", ImVec2(S(110), 0), true)) { app.ResetAllSettings(); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine(0, S(8));
        if (ui::Button("Cancel", ImVec2(S(110), 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ui::EndCard();

    ui::BeginCard("Game Profiles");
    {
        const Theme& th = ThemeManager::Get().T();
        ui::NoteText("While a listed program is running, its crosshair, HUD and keybinds are applied automatically. Hatenium only looks at the list of running program names.");
        ImGui::Dummy(ImVec2(0, S(2)));
        static int sel = -1;
        int active = app.profiles().ActiveIndex();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.25f));
        ImGui::BeginChild("##profiles", ImVec2(0, S(96)), ImGuiChildFlags_None, 0);
        ImGui::PopStyleColor();
        if (s.profiles.empty()) ImGui::TextColored(th.textMuted, "%s", tr("No profiles yet."));
        for (int i = 0; i < static_cast<int>(s.profiles.size()); ++i) {
            ImGui::PushID(i);
            std::string label = std::string(i == active ? "> " : "   ") + s.profiles[static_cast<size_t>(i)].name + "  (" + s.profiles[static_cast<size_t>(i)].exe + ")" +
                                (s.profiles[static_cast<size_t>(i)].enabled ? "" : "  [off]");
            if (i == active) ImGui::PushStyleColor(ImGuiCol_Text, th.accent);
            if (ImGui::Selectable(label.c_str(), sel == i)) sel = i;
            if (i == active) ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::EndChild();
        if (sel >= static_cast<int>(s.profiles.size())) sel = -1;

        static char pname[48] = "", pexe[96] = "";
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##pname", tr("Profile name"), pname, sizeof(pname));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##pexe", tr("Program, e.g. RobloxPlayerBeta.exe"), pexe, sizeof(pexe));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##running", tr("Pick a running program"))) {
            int shown = 0;
            for (const auto& n : app.profiles().Processes()) {
                if (++shown > 400) break;
                if (ImGui::Selectable(n.c_str())) { std::snprintf(pexe, sizeof(pexe), "%s", n.c_str()); if (!pname[0]) std::snprintf(pname, sizeof(pname), "%s", n.c_str()); }
            }
            ImGui::EndCombo();
        }
        float bw = (ImGui::GetContentRegionAvail().x - S(8)) * 0.5f;
        if (ui::Button("Add Profile", ImVec2(bw, 0), true) && pexe[0] && s.profiles.size() < 50) {
            GameProfile p; p.name = pname[0] ? pname : pexe; p.exe = pexe;
            ProfileManager::CaptureInto(p, s);
            s.profiles.push_back(p); sel = static_cast<int>(s.profiles.size()) - 1; pname[0] = pexe[0] = 0;
        }
        ImGui::SameLine(0, S(8));
        if (ui::Button("Add Roblox", ImVec2(bw, 0)) && s.profiles.size() < 50) {
            GameProfile p; p.name = "Roblox"; p.exe = "RobloxPlayerBeta.exe";
            ProfileManager::CaptureInto(p, s);
            s.profiles.push_back(p); sel = static_cast<int>(s.profiles.size()) - 1;
        }
        if (sel >= 0) {
            GameProfile& p = s.profiles[static_cast<size_t>(sel)];
            ui::Checkbox("Profile Enabled", &p.enabled);
            if (ui::Button("Save Current to Profile", ImVec2(bw, 0))) ProfileManager::CaptureInto(p, s);
            ImGui::SameLine(0, S(8));
            if (ui::Button("Delete Profile", ImVec2(bw, 0))) { s.profiles.erase(s.profiles.begin() + sel); sel = -1; }
        }
    }
    ui::EndCard();

    ui::BeginCard("Configuration");
    {
        float avail = ImGui::GetContentRegionAvail().x, gap = S(8);
        int perRow = avail < S(380) ? 2 : 5;
        float bw2 = (avail - gap * (perRow - 1)) / perRow;
        const char* names[5] = {"Save", "Load", "Reset", "Export", "Import"};
        for (int i = 0; i < 5; ++i) {
            if (i % perRow) ImGui::SameLine(0, gap);
            if (ui::Button(names[i], ImVec2(bw2, 0), i == 0)) {
                switch (i) {
                case 0: app.SaveSettings(true); break;
                case 1: app.LoadSettings(); break;
                case 2: app.ResetFeatureSettings(); break;
                case 3: app.ExportSettings(); break;
                default: app.ImportSettings(); break;
                }
            }
        }
    }
    ui::EndCard();

    ui::EndColumns();
}

} // namespace hat::gui
