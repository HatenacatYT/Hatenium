#pragma once
#include "imgui.h"
#include "core/Settings.h"

namespace hat {

struct Theme {
    ImVec4 accent{0.f, 0.9f, 1.f, 1.f};
    ImVec4 bg, panel, card, cardHover, frame, border, borderBright, text, textDim, textMuted;
    float buttonRounding = 8.f, windowRounding = 14.f, cardRounding = 8.f;   // already scaled (pixels)
    float scale = 1.f;
    float panelOpacity = 0.92f;
};

// Holds the live (smoothed) theme and pushes it into the ImGui style every frame.
class ThemeManager {
public:
    static ThemeManager& Get();
    static ImVec4 AccentFor(const Settings& s);
    static ImVec4 PresetColor(int index);              // 0..4
    static float  ButtonRoundingFor(int shape);        // logical px
    static float  WindowRoundingFor(int appearance);   // logical px

    void SetScale(float s) { t_.scale = s; }
    void SetLogoFont(ImFont* f) { logoFont_ = f; }
    ImFont* LogoFont() const { return logoFont_; }
    void Update(const Settings& s, float dt, bool snap = false);
    const Theme& T() const { return t_; }

private:
    void ApplyStyle();
    Theme t_;
    bool init_ = false;
    float btnLogical_ = 8.f, winLogical_ = 14.f;
    ImFont* logoFont_ = nullptr;
};

inline float S(float v) { return v * ThemeManager::Get().T().scale; }
inline ImVec2 S(float x, float y) { return ImVec2(S(x), S(y)); }
inline ImVec4 WithAlpha(ImVec4 c, float a) { c.w *= a; return c; }
inline ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}
// Colour -> packed, honouring the current ImGui style alpha (so fades apply to custom drawing too).
inline ImU32 C32(const ImVec4& c) { return ImGui::GetColorU32(c); }
inline ImU32 Raw32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

} // namespace hat
