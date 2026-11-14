#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"
#include <algorithm>

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
    ImGui::TableNextColumn();
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
    ui::Checkbox("Remember Position", &s.rememberPosition);
    ui::Checkbox("Remember Size", &s.rememberSize);
    ui::EndCard();

    // ---------------------------------------------------------- column 2
    ImGui::TableNextColumn();
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
