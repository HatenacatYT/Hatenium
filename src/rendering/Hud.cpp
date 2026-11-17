#include "rendering/Hud.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace hat {

namespace {
bool g_resetRequested = false;

struct KeyDef { const char* label; int vk; float col, row, w; };
const KeyDef kKeys[15] = {
    {"W", 'W', 1, 0, 1},       {"A", 'A', 0, 1, 1},     {"S", 'S', 1, 1, 1},      {"D", 'D', 2, 1, 1},
    {"SPACE", VK_SPACE, 0, 3, 3}, {"SHIFT", VK_SHIFT, 0, 2, 1.5f}, {"CTRL", VK_CONTROL, 1.5f, 2, 1.5f},
    {"LMB", VK_LBUTTON, 0, 4, 1.5f}, {"RMB", VK_RBUTTON, 1.5f, 4, 1.5f},
    {"R", 'R', 3, 0, 1},       {"E", 'E', 2, 0, 1},     {"Q", 'Q', 0, 0, 1},      {"F", 'F', 3, 1, 1},
    {"M4", VK_XBUTTON1, 3, 3, 1}, {"M5", VK_XBUTTON2, 3, 4, 1},
};

bool Enabled(const KeyOverlaySettings& k, int i) {
    switch (i) {
    case 0: return k.w; case 1: return k.a; case 2: return k.s; case 3: return k.d; case 4: return k.space;
    case 5: return k.shift; case 6: return k.ctrl; case 7: return k.lmb; case 8: return k.rmb;
    case 9: return k.r; case 10: return k.e; case 11: return k.q; case 12: return k.f;
    case 13: return k.side1; default: return k.side2;
    }
}

Rgba Mix(const Rgba& a, const Rgba& b, float t) {
    return Rgba{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}
} // namespace

void RequestCpsReset() { g_resetRequested = true; }

bool HudRenderer::KeyDown(int i) const { return (GetAsyncKeyState(kKeys[i].vk) & 0x8000) != 0; }

void HudRenderer::ResetCps() { clicks_.clear(); cps_ = 0; maxCps_ = 0; }

void HudRenderer::Update(float dt, const HudSettings& s) {
    clock_ += dt;
    if (g_resetRequested) { g_resetRequested = false; ResetCps(); }

    if (s.keys.enable) {
        for (int i = 0; i < 15; ++i) {
            float target = (Enabled(s.keys, i) && KeyDown(i)) ? 1.f : 0.f;
            keyAnim_[i] += (target - keyAnim_[i]) * (1.f - std::exp(-24.f * dt));
            if (std::fabs(target - keyAnim_[i]) < 0.003f) keyAnim_[i] = target;
        }
    }
    bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (s.cps.enable) {
        if (lmb && !prevLmb_) clicks_.push_back(clock_);
        while (!clicks_.empty() && clock_ - clicks_.front() > 1.f) clicks_.pop_front();
        cps_ = static_cast<int>(clicks_.size());
        maxCps_ = std::max(maxCps_, cps_);
    }
    prevLmb_ = lmb;
}

std::vector<HudRenderer::Box> HudRenderer::Bounds(const HudSettings& s, Vec2 origin, Vec2 size) const {
    std::vector<Box> out;
    if (s.keys.enable) {
        const auto& k = s.keys;
        float u = 42.f * k.scale, gap = 5.f * k.scale;
        float ox = origin.x + k.posX * 0.01f * size.x, oy = origin.y + k.posY * 0.01f * size.y;
        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
        for (int i = 0; i < 15; ++i) {
            if (!Enabled(k, i)) continue;
            const KeyDef& d = kKeys[i];
            float x = ox + d.col * (u + gap), y = oy + d.row * (u + gap);
            float w = d.w * u + (d.w - 1.f) * gap, h = (d.row >= 4 ? u * 0.85f : u);
            x0 = std::min(x0, x); y0 = std::min(y0, y); x1 = std::max(x1, x + w); y1 = std::max(y1, y + h);
        }
        if (x1 > x0) out.push_back({0, Vec2{x0, y0}, Vec2{x1, y1}});
        else out.push_back({0, Vec2{ox, oy}, Vec2{ox + u, oy + u}});
    }
    if (s.cps.enable) {
        float x = origin.x + s.cps.posX * 0.01f * size.x, y = origin.y + s.cps.posY * 0.01f * size.y;
        out.push_back({1, Vec2{x, y}, Vec2{x + 118.f * s.cps.scale, y + (s.cps.showMax ? 62.f : 40.f) * s.cps.scale}});
    }
    if (s.timer.enable) {
        float x = origin.x + s.timer.posX * 0.01f * size.x, y = origin.y + s.timer.posY * 0.01f * size.y;
        out.push_back({2, Vec2{x, y}, Vec2{x + 130.f * s.timer.scale, y + 42.f * s.timer.scale}});
    }
    return out;
}

void HudRenderer::Render(DrawSink& sink, const HudSettings& s, Vec2 origin, Vec2 size, const std::string& timerText) const {
    float bt = s.borderThickness;
    Rgba bgc{s.background[0], s.background[1], s.background[2], 1.f};
    // ---------------------------------------------------------------- key strokes
    if (s.keys.enable) {
        const auto& k = s.keys;
        float u = 42.f * k.scale, gap = 5.f * k.scale, rad = 8.f * k.scale;
        float ox = origin.x + k.posX * 0.01f * size.x, oy = origin.y + k.posY * 0.01f * size.y;
        Rgba act{k.activeColor[0], k.activeColor[1], k.activeColor[2], k.opacity};
        Rgba ina{k.inactiveColor[0], k.inactiveColor[1], k.inactiveColor[2], k.opacity};
        Rgba txt{k.textColor[0], k.textColor[1], k.textColor[2], 1.f};
        float lum = 0.299f * act.r + 0.587f * act.g + 0.114f * act.b;
        Rgba dark{0.02f, 0.04f, 0.07f, 1.f};
        for (int i = 0; i < 15; ++i) {
            if (!Enabled(k, i)) continue;
            const KeyDef& d = kKeys[i];
            float t = keyAnim_[i];
            float x = ox + d.col * (u + gap), y = oy + d.row * (u + gap);
            float w = d.w * u + (d.w - 1.f) * gap, h = (d.row >= 4 ? u * 0.85f : u);
            Rgba border = Mix(Rgba{act.r, act.g, act.b, 0.35f * k.opacity}, Rgba{act.r, act.g, act.b, 1.f}, t);
            sink.RoundRect(Vec2{x, y}, Vec2{x + w, y + h}, rad, Mix(ina, act, t), border, bt > 0.f ? std::max(1.f, bt * k.scale) : 0.f);
            if (k.showLabels) {
                Rgba tc = Mix(txt, lum > 0.55f ? dark : Rgba{1, 1, 1, 1}, t);
                sink.Text(Vec2{x + w * 0.5f, y + h * 0.5f}, d.label, (d.w >= 1.5f ? 12.f : 15.f) * k.scale, tc, 1, true);
            }
        }
    }
    // ---------------------------------------------------------------- CPS
    if (s.cps.enable) {
        const auto& c = s.cps;
        float sc = c.scale;
        float x = origin.x + c.posX * 0.01f * size.x, y = origin.y + c.posY * 0.01f * size.y;
        float w = 118.f * sc, h = (c.showMax ? 62.f : 40.f) * sc;
        Rgba col{c.color[0], c.color[1], c.color[2], c.opacity};
        sink.RoundRect(Vec2{x, y}, Vec2{x + w, y + h}, 9.f * sc, Rgba{bgc.r, bgc.g, bgc.b, 0.88f * c.opacity},
                       Rgba{col.r, col.g, col.b, 0.55f}, bt > 0.f ? std::max(1.f, bt * sc) : 0.f);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d CPS", cps_);
        sink.Text(Vec2{x + w * 0.5f, y + 20.f * sc}, buf, 20.f * sc, col, 1, true);
        if (c.showMax) {
            std::snprintf(buf, sizeof(buf), "MAX %d", maxCps_);
            sink.Text(Vec2{x + w * 0.5f, y + 45.f * sc}, buf, 13.f * sc, Rgba{0.7f, 0.76f, 0.86f, c.opacity}, 1, false);
        }
    }
    // ---------------------------------------------------------------- timer / stopwatch
    if (s.timer.enable) {
        const auto& t = s.timer;
        float x = origin.x + t.posX * 0.01f * size.x, y = origin.y + t.posY * 0.01f * size.y;
        float w = 130.f * t.scale, h = 42.f * t.scale;
        Rgba col{s.cps.color[0], s.cps.color[1], s.cps.color[2], s.cps.opacity};
        sink.RoundRect(Vec2{x, y}, Vec2{x + w, y + h}, 9.f * t.scale, Rgba{bgc.r, bgc.g, bgc.b, 0.88f * s.cps.opacity},
                       Rgba{col.r, col.g, col.b, 0.55f}, bt > 0.f ? std::max(1.f, bt * t.scale) : 0.f);
        sink.Text(Vec2{x + w * 0.5f, y + h * 0.5f}, timerText.c_str(), 22.f * t.scale, col, 1, true);
    }
}

} // namespace hat
