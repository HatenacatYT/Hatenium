#include "gui/ThemeManager.h"
#include "core/AnimationManager.h"
#include <algorithm>
#include <cmath>

namespace hat {

namespace {
const ImVec4 kPresets[5] = {
    {0.00f, 0.90f, 1.00f, 1.f},   // Cyan
    {0.58f, 0.38f, 1.00f, 1.f},   // Purple
    {1.00f, 0.27f, 0.33f, 1.f},   // Red
    {0.20f, 0.95f, 0.50f, 1.f},   // Green
    {0.93f, 0.96f, 1.00f, 1.f},   // White
};
} // namespace

ThemeManager& ThemeManager::Get() { static ThemeManager t; return t; }

ImVec4 ThemeManager::PresetColor(int i) { return kPresets[std::min(std::max(i, 0), 4)]; }

ImVec4 ThemeManager::AccentFor(const Settings& s) {
    if (s.theme >= 0 && s.theme < 5) return kPresets[s.theme];
    return ImVec4(s.customColor[0], s.customColor[1], s.customColor[2], 1.f);
}

float ThemeManager::ButtonRoundingFor(int shape) {
    static const float v[4] = {0.f, 4.f, 8.f, 16.f};   // 16 = pill (ImGui clamps to half the height)
    return v[std::min(std::max(shape, 0), 3)];
}

float ThemeManager::WindowRoundingFor(int a) {
    static const float v[4] = {0.f, 8.f, 14.f, 22.f};
    return v[std::min(std::max(a, 0), 3)];
}

void ThemeManager::Update(const Settings& s, float dt, bool snap) {
    ImVec4 ta = AccentFor(s);
    float tb = ButtonRoundingFor(s.buttonShape), tw = WindowRoundingFor(s.windowAppearance);
    float k = 1.f;
    if (init_ && !snap && AnimationManager::Get().Enabled())
        k = 1.f - std::exp(-10.f * AnimationManager::Get().Speed() * dt);
    if (!init_) { t_.accent = ta; btnLogical_ = tb; winLogical_ = tw; }
    else {
        t_.accent = Mix(t_.accent, ta, k);
        btnLogical_ += (tb - btnLogical_) * k;
        winLogical_ += (tw - winLogical_) * k;
    }
    init_ = true;

    t_.bg           = ImVec4(0.031f, 0.043f, 0.071f, 1.f);
    t_.panel        = ImVec4(0.047f, 0.063f, 0.098f, 1.f);
    t_.card         = ImVec4(0.063f, 0.082f, 0.125f, 1.f);
    t_.cardHover    = ImVec4(0.086f, 0.110f, 0.165f, 1.f);
    t_.frame        = ImVec4(0.040f, 0.055f, 0.090f, 1.f);
    t_.border       = ImVec4(0.118f, 0.153f, 0.231f, 1.f);
    t_.borderBright = ImVec4(0.190f, 0.240f, 0.350f, 1.f);
    t_.text         = ImVec4(0.91f, 0.94f, 0.98f, 1.f);
    t_.textDim      = ImVec4(0.60f, 0.66f, 0.76f, 1.f);
    t_.textMuted    = ImVec4(0.38f, 0.43f, 0.54f, 1.f);
    t_.panelOpacity = s.misc.panelOpacity;
    t_.buttonRounding = btnLogical_ * t_.scale;
    t_.windowRounding = winLogical_ * t_.scale;
    t_.cardRounding   = std::min(winLogical_, 10.f) * t_.scale;
    ApplyStyle();
}

void ThemeManager::ApplyStyle() {
    const Theme& t = t_;
    ImGuiStyle& st = ImGui::GetStyle();
    st = ImGuiStyle();
    st.ScaleAllSizes(t.scale);

    st.WindowPadding = S(14, 12);
    st.FramePadding = S(10, 6);
    st.ItemSpacing = S(8, 8);
    st.ItemInnerSpacing = S(6, 4);
    st.CellPadding = S(6, 4);
    st.ScrollbarSize = S(8);
    st.GrabMinSize = S(10);
    st.WindowBorderSize = 0.f;
    st.ChildBorderSize = 1.f;
    st.PopupBorderSize = 1.f;
    st.FrameBorderSize = 0.f;
    st.WindowRounding = t.windowRounding;
    st.ChildRounding = t.cardRounding;
    st.FrameRounding = t.buttonRounding;
    st.PopupRounding = std::min(t.buttonRounding, S(8));
    st.GrabRounding = t.buttonRounding;
    st.ScrollbarRounding = S(4);
    st.DisabledAlpha = 1.f;   // disabled (busy) UI is not greyed out, it just ignores input

    ImVec4* c = st.Colors;
    const ImVec4& a = t.accent;
    c[ImGuiCol_Text] = t.text;
    c[ImGuiCol_TextDisabled] = t.textMuted;
    c[ImGuiCol_WindowBg] = t.bg;
    c[ImGuiCol_ChildBg] = WithAlpha(t.card, t.panelOpacity);
    c[ImGuiCol_PopupBg] = WithAlpha(t.card, 0.98f);
    c[ImGuiCol_Border] = t.border;
    c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg] = t.frame;
    c[ImGuiCol_FrameBgHovered] = t.cardHover;
    c[ImGuiCol_FrameBgActive] = t.cardHover;
    c[ImGuiCol_Button] = t.cardHover;
    c[ImGuiCol_ButtonHovered] = Mix(t.cardHover, a, 0.25f);
    c[ImGuiCol_ButtonActive] = Mix(t.cardHover, a, 0.4f);
    c[ImGuiCol_Header] = WithAlpha(a, 0.18f);
    c[ImGuiCol_HeaderHovered] = WithAlpha(a, 0.28f);
    c[ImGuiCol_HeaderActive] = WithAlpha(a, 0.38f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = t.borderBright;
    c[ImGuiCol_ScrollbarGrabHovered] = Mix(t.borderBright, a, 0.5f);
    c[ImGuiCol_ScrollbarGrabActive] = a;
    c[ImGuiCol_CheckMark] = a;
    c[ImGuiCol_SliderGrab] = a;
    c[ImGuiCol_SliderGrabActive] = a;
    c[ImGuiCol_Separator] = t.border;
    c[ImGuiCol_TextSelectedBg] = WithAlpha(a, 0.35f);
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.55f);
}

} // namespace hat
