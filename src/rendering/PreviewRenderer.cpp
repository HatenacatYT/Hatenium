#include "rendering/PreviewRenderer.h"
#include "rendering/Effects.h"
#include "rendering/OverlayWindow.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"
#include "core/AnimationManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace hat {

namespace {
ImU32 Rgba3(const Col3& c, float a) { return ImGui::GetColorU32(ImVec4(c[0], c[1], c[2], a)); }

class ImGuiSink : public DrawSink {
public:
    ImGuiSink(ImDrawList* dl, ImVec2 origin) : dl_(dl), o_(origin) {}
    void Line(Vec2 a, Vec2 b, float th, Rgba c) override {
        dl_->AddLine(P(a), P(b), U(c), std::max(1.f, th));
    }
    void Circle(Vec2 p, float r, Rgba c, bool filled, float th) override {
        if (filled) dl_->AddCircleFilled(P(p), std::max(0.5f, r), U(c), 24);
        else dl_->AddCircle(P(p), std::max(0.5f, r), U(c), 32, std::max(1.f, th));
    }
    void Polygon(const Vec2* pts, int n, Rgba c, bool filled, float th) override {
        if (!filled) { for (int i = 0; i < n; ++i) dl_->AddLine(P(pts[i]), P(pts[(i + 1) % n]), U(c), std::max(1.f, th)); return; }
        ImVec2 ctr(0, 0);
        for (int i = 0; i < n; ++i) { ctr.x += pts[i].x; ctr.y += pts[i].y; }
        ctr.x /= n; ctr.y /= n;
        ImVec2 cc(ctr.x + o_.x, ctr.y + o_.y);
        for (int i = 0; i < n; ++i) dl_->AddTriangleFilled(cc, P(pts[i]), P(pts[(i + 1) % n]), U(c));   // star-shaped fan
    }
    void RoundRect(Vec2 a, Vec2 b, float r, Rgba fill, Rgba border, float bt) override {
        dl_->AddRectFilled(P(a), P(b), U(fill), r);
        if (bt > 0.f) dl_->AddRect(P(a), P(b), U(border), r, 0, bt);
    }
    void Text(Vec2 p, const char* t, float px, Rgba c, int align, bool) override {
        ImFont* f = ImGui::GetFont();
        ImVec2 sz = f->CalcTextSizeA(px, FLT_MAX, 0.f, t);
        float x = align == 1 ? p.x - sz.x * 0.5f : p.x;
        dl_->AddText(f, px, ImVec2(x + o_.x, p.y - sz.y * 0.5f + o_.y), U(c), t);
    }
private:
    ImVec2 P(Vec2 v) const { return ImVec2(v.x + o_.x, v.y + o_.y); }
    static ImU32 U(const Rgba& c) { return ImGui::GetColorU32(ImVec4(c.r, c.g, c.b, c.a)); }
    ImDrawList* dl_;
    ImVec2 o_;
};

EffectsEngine g_previewEngine;
}

void DrawVisualPreview(ImDrawList* dl, ImVec2 a, ImVec2 b, const VisualSettings& v, float time) {
    const Theme& th = ThemeManager::Get().T();
    float rounding = std::min(th.cardRounding, S(10));
    float w = b.x - a.x, h = b.y - a.y;

    dl->AddRectFilled(a, b, C32(ImVec4(0.020f, 0.028f, 0.048f, 1.f)), rounding);

    float inset = rounding * 0.3f;
    dl->PushClipRect(ImVec2(a.x + inset, a.y + inset), ImVec2(b.x - inset, b.y - inset), true);

    // subtle perspective-less grid + horizon glow
    float step = S(30);
    ImU32 grid = C32(WithAlpha(th.borderBright, 0.28f));
    for (float x = a.x + std::fmod(w * 0.5f, step); x < b.x; x += step) dl->AddLine(ImVec2(x, a.y), ImVec2(x, b.y), grid);
    for (float y = a.y + std::fmod(h * 0.5f, step); y < b.y; y += step) dl->AddLine(ImVec2(a.x, y), ImVec2(b.x, y), grid);
    dl->AddRectFilledMultiColor(ImVec2(a.x, a.y + h * 0.5f - S(26)), ImVec2(b.x, a.y + h * 0.5f),
        C32(WithAlpha(th.accent, 0.0f)), C32(WithAlpha(th.accent, 0.0f)), C32(WithAlpha(th.accent, 0.07f)), C32(WithAlpha(th.accent, 0.07f)));

    ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    float rs = h / static_cast<float>(OverlayWindow::PrimaryScreenHeight());   // canvas = scaled-down screen, so it matches the real overlay

    // ---- FOV circle ----
    const auto& f = v.fovCircle;
    if (f.enable) {
        ImVec2 fc(c.x + f.centerX * 0.01f * w * 0.5f, c.y + f.centerY * 0.01f * h * 0.5f);
        float r = f.radius * rs;
        if (f.filled) dl->AddCircleFilled(fc, r, Rgba3(f.color, f.opacity * 0.20f), 128);
        if (f.outline) dl->AddCircle(fc, r, Rgba3(f.color, f.opacity), 128, std::max(1.f, f.thickness * rs));
    }

    // ---- crosshair ----
    const auto& ch = v.crosshair;
    if (ch.enable) {
        float size = ch.size * rs, gap = ch.gap * rs, th2 = std::max(1.f, ch.thickness * rs);
        ImU32 col = Rgba3(ch.color, ch.opacity);
        bool cross = (ch.style == 0 || ch.style == 3);
        if (cross) {
            dl->AddLine(ImVec2(c.x - gap - size, c.y), ImVec2(c.x - gap, c.y), col, th2);
            dl->AddLine(ImVec2(c.x + gap, c.y), ImVec2(c.x + gap + size, c.y), col, th2);
            dl->AddLine(ImVec2(c.x, c.y - gap - size), ImVec2(c.x, c.y - gap), col, th2);
            dl->AddLine(ImVec2(c.x, c.y + gap), ImVec2(c.x, c.y + gap + size), col, th2);
        }
        if (ch.style == 1 || ch.style == 3) dl->AddCircleFilled(c, std::max(1.5f, th2 * 0.9f), col, 16);
        if (ch.style == 2) dl->AddCircle(c, gap + size * 0.5f, col, 48, th2);
    }
    // ---- mouse trail + click particles follow the real mouse while it is over the canvas ----
    {
        ImGuiIO& io = ImGui::GetIO();
        bool over = ImGui::IsMouseHoveringRect(a, b) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        Vec2 cur{io.MousePos.x - a.x, io.MousePos.y - a.y};
        g_previewEngine.Update(io.DeltaTime, cur, over, over && io.MouseDown[0], over && io.MouseDown[1], v.trail, v.particles, rs);
        ImGuiSink sink(dl, a);
        g_previewEngine.Render(sink, v.trail, rs);
        g_previewEngine.RenderParticles(sink, rs);
        if (over && (v.trail.enable || v.particles.enable)) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    dl->PopClipRect();

    dl->AddRect(a, b, C32(th.border), rounding);
    dl->AddText(ImVec2(a.x + S(10), a.y + S(8)), C32(th.textMuted), tr("Preview"));
    (void)time;
}

void DrawSpeedPreview(ImDrawList* dl, ImVec2 a, ImVec2 b, float time) {
    const Theme& th = ThemeManager::Get().T();
    float rounding = std::min(th.cardRounding, S(10));
    dl->AddRectFilled(a, b, C32(ImVec4(0.020f, 0.028f, 0.048f, 1.f)), rounding);
    dl->AddRect(a, b, C32(th.border), rounding);

    float speed = 260.f + 150.f * std::sin(time * 1.7f) + 40.f * std::sin(time * 4.1f);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%03d", static_cast<int>(speed));
    ImFont* f = ImGui::GetFont();
    float fs = f->FontSize * 2.0f;
    ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0.f, buf);
    float cx = (a.x + b.x) * 0.5f;
    dl->AddText(f, fs, ImVec2(cx - ts.x * 0.5f, a.y + S(8)), C32(th.accent), buf);
    dl->AddText(ImVec2(cx + ts.x * 0.5f + S(6), a.y + S(8) + ts.y - ImGui::GetFontSize() - S(4)), C32(th.textMuted), "u/s");

    float bx0 = a.x + S(14), bx1 = b.x - S(14), by = b.y - S(18);
    dl->AddRectFilled(ImVec2(bx0, by), ImVec2(bx1, by + S(5)), C32(th.frame), S(3));
    float t = Clamp01(speed / 450.f);
    dl->AddRectFilled(ImVec2(bx0, by), ImVec2(bx0 + (bx1 - bx0) * t, by + S(5)), C32(th.accent), S(3));
    const char* lbl = tr("Simulated speed");
    ImVec2 ls = ImGui::CalcTextSize(lbl);
    dl->AddText(ImVec2(a.x + (b.x - a.x - ls.x) * 0.5f, by - ls.y - S(4)), C32(th.textMuted), lbl);
}

} // namespace hat
