#include "gui/Screens.h"
#include "gui/ThemeManager.h"
#include "gui/Components.h"
#include "core/AnimationManager.h"
#include "core/Localization.h"
#include <algorithm>
#include <cmath>

namespace hat::gui {

// Draws text one glyph at a time with extra letter spacing; returns the total width.
static float SpacedTextWidth(ImFont* f, float size, const char* s, float spacing) {
    float w = 0;
    for (const char* c = s; *c; ++c) w += f->CalcTextSizeA(size, FLT_MAX, 0.f, c, c + 1).x + spacing;
    return w - spacing;
}
static void DrawSpacedText(ImDrawList* dl, ImFont* f, float size, ImVec2 p, ImU32 col, const char* s, float spacing) {
    for (const char* c = s; *c; ++c) {
        dl->AddText(f, size, p, col, c, c + 1);
        p.x += f->CalcTextSizeA(size, FLT_MAX, 0.f, c, c + 1).x + spacing;
    }
}

void DrawSplash(Application& app) {
    const Theme& th = ThemeManager::Get().T();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize();
    float t = AnimationManager::Get().Time();
    ImVec2 c(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);
    float u = std::min(sz.x / 640.f, sz.y / 360.f);   // layout unit follows the window size

    // soft accent glow behind the logo
    for (int i = 0; i < 7; ++i) {
        float r = (150.f - i * 18.f) * u;
        dl->AddCircleFilled(ImVec2(c.x, c.y - 28.f * u), r, C32(WithAlpha(th.accent, 0.012f + i * 0.0025f)), 64);
    }

    // logo: halo ring + HATENIUM
    ImVec2 halo(c.x, c.y - 66.f * u);
    float pulse = 0.5f + 0.5f * std::sin(t * 2.2f);
    dl->AddCircle(halo, 17.f * u, C32(WithAlpha(th.accent, 0.12f + 0.10f * pulse)), 48, 7.f * u);
    dl->AddCircle(halo, 17.f * u, C32(th.accent), 48, 2.4f * u);

    ImFont* logo = ThemeManager::Get().LogoFont();
    if (!logo) logo = ImGui::GetFont();
    float fs = logo->FontSize * std::min(1.25f, std::max(0.6f, u / th.scale));
    float spacing = fs * 0.22f;
    float tw = SpacedTextWidth(logo, fs, "HATENIUM", spacing);
    ImVec2 tp(c.x - tw * 0.5f, c.y - 36.f * u);
    DrawSpacedText(dl, logo, fs, ImVec2(tp.x + 1.5f, tp.y + 1.5f), C32(WithAlpha(th.accent, 0.25f)), "HATENIUM", spacing);
    DrawSpacedText(dl, logo, fs, tp, C32(th.text), "HATENIUM", spacing);

    // accent underline
    float lw = tw * 0.46f;
    dl->AddRectFilled(ImVec2(c.x - lw * 0.5f, tp.y + fs + 6.f * u), ImVec2(c.x + lw * 0.5f, tp.y + fs + 8.f * u), C32(th.accent), 1.f);

    // spinner
    ImVec2 sp(c.x, c.y + 52.f * u);
    float sr = 12.f * u;
    dl->AddCircle(sp, sr, C32(WithAlpha(th.accent, 0.15f)), 40, 2.4f * u);
    float a0 = t * 5.f;
    dl->PathClear();
    dl->PathArcTo(sp, sr, a0, a0 + 1.9f + 0.7f * std::sin(t * 3.f), 28);
    dl->PathStroke(C32(th.accent), 0, 2.6f * u);

    // status + progress
    const char* status = tr(app.SplashStatus());
    ImVec2 ts = ImGui::CalcTextSize(status);
    dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y + 82.f * u), C32(th.textDim), status);
    float bw = 220.f * u, by = pos.y + sz.y - 40.f * u;
    dl->AddRectFilled(ImVec2(c.x - bw * 0.5f, by), ImVec2(c.x + bw * 0.5f, by + 3.f * u), C32(th.frame), 2.f);
    dl->AddRectFilled(ImVec2(c.x - bw * 0.5f, by), ImVec2(c.x - bw * 0.5f + bw * app.SplashProgress(), by + 3.f * u), C32(th.accent), 2.f);

    const char* ver = "v" HATENIUM_VERSION;
    ImVec2 vs = ImGui::CalcTextSize(ver);
    dl->AddText(ImVec2(pos.x + sz.x - vs.x - 14.f * u, pos.y + sz.y - vs.y - 12.f * u), C32(th.textMuted), ver);
}

} // namespace hat::gui
