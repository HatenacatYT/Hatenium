#include "gui/Screens.h"
#include "gui/ThemeManager.h"
#include "gui/Components.h"
#include "core/Localization.h"
#include <algorithm>

namespace hat::gui {

namespace {
const char* kThemeNames[6] = {"Cyan", "Purple", "Red", "Green", "White", "Custom"};
const char* kShapeIds[4] = {"btn_0", "btn_1", "btn_2", "btn_3"};
const char* kWindowIds[4] = {"win_0", "win_1", "win_2", "win_3"};
const char* kShapeShort[4] = {"Square", "Slight", "Rounded", "Pill"};
const char* kWindowShort[4] = {"Sharp", "Soft", "Rounded", "Extra"};

void SectionLabel(const char* text) {
    const Theme& th = ThemeManager::Get().T();
    ImGui::Dummy(ImVec2(0, S(4)));
    ImGui::TextColored(th.textDim, "%s", tr(text));
    ImGui::Dummy(ImVec2(0, S(2)));
}
} // namespace

void DrawSetup(Application& app) {
    const Theme& th = ThemeManager::Get().T();
    Settings& s = app.settings();
    ImVec2 size = ImGui::GetWindowSize();
    float pad = S(20), footerH = S(64);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, S(18)));
    ImGui::BeginChild("##setup_scroll", ImVec2(size.x, size.y - footerH), ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();

    // header
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float availW = ImGui::GetContentRegionAvail().x;
    ImFont* f = ImGui::GetFont();
    dl->AddCircle(ImVec2(p.x + S(10), p.y + S(12)), S(8), C32(th.accent), 32, S(2));
    dl->AddText(f, f->FontSize * 1.35f, ImVec2(p.x + S(28), p.y), C32(th.text), "HATENIUM");
    ImGui::Dummy(ImVec2(0, f->FontSize * 1.35f + S(4)));
    ImGui::TextColored(th.text, "%s", tr("Appearance Setup"));
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextColored(th.textMuted, "%s", tr("Choose how Hatenium looks. You can change this any time in Settings."));
    ImGui::PopTextWrapPos();

    // ---- theme colour ----
    SectionLabel("Theme Color");
    float gap = S(6);
    float d = std::min(S(36), (availW - gap * 5.f) / 6.f);
    for (int i = 0; i < 6; ++i) {
        if (i) ImGui::SameLine(0, gap);
        ImVec4 col = (i < 5) ? ThemeManager::PresetColor(i)
                             : ImVec4(s.customColor[0], s.customColor[1], s.customColor[2], 1.f);
        if (ui::Swatch(kThemeNames[i], col, s.theme == i, d)) s.theme = i;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tr(kThemeNames[i]));
    }
    if (s.theme == 5) {
        ImGui::Dummy(ImVec2(0, S(4)));
        ImGui::SetNextItemWidth(availW);
        ImGui::ColorPicker3("##custom", s.customColor.data(),
            ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    }

    // ---- button shape ----
    SectionLabel("Button Shape");
    float tg = S(6), tw = (availW - tg * 3.f) / 4.f;
    static const float shapeR[4] = {0.f, 4.f, 8.f, 16.f};
    for (int i = 0; i < 4; ++i) {
        if (i) ImGui::SameLine(0, tg);
        if (ui::ShapeTile(kShapeIds[i], kShapeShort[i], s.buttonShape == i, shapeR[i], 0, tw)) s.buttonShape = i;
    }

    // ---- window appearance ----
    SectionLabel("Window Appearance");
    static const float winR[4] = {0.f, 8.f, 14.f, 22.f};
    for (int i = 0; i < 4; ++i) {
        if (i) ImGui::SameLine(0, tg);
        if (ui::ShapeTile(kWindowIds[i], kWindowShort[i], s.windowAppearance == i, winR[i], 1, tw)) s.windowAppearance = i;
    }

    // ---- live preview ----
    SectionLabel("Live Preview");
    static bool demoToggle = true;
    static float demoSlider = 60.f;
    ui::BeginCard("Live Preview");
    float bw = (ImGui::GetContentRegionAvail().x - S(8)) * 0.5f;
    ui::Button("Primary", ImVec2(bw, 0), true);
    ImGui::SameLine(0, S(8));
    ui::Button("Secondary", ImVec2(bw, 0), false);
    ui::Checkbox("Sample toggle", &demoToggle);
    ui::SliderFloat("Sample slider", &demoSlider, 0.f, 100.f, "%.0f");
    ui::EndCard();
    ImGui::Dummy(ImVec2(0, S(6)));
    ImGui::EndChild();

    // ---- footer ----
    ImVec2 wp = ImGui::GetWindowPos();
    ImGui::SetCursorScreenPos(ImVec2(wp.x + pad, wp.y + size.y - footerH + S(10)));
    dl = ImGui::GetWindowDrawList();
    dl->AddLine(ImVec2(wp.x + pad, wp.y + size.y - footerH), ImVec2(wp.x + size.x - pad, wp.y + size.y - footerH), C32(th.border));
    if (ui::Button("Finish Setup", ImVec2(size.x - pad * 2.f, S(38)), true)) app.FinishSetup();
}

} // namespace hat::gui
