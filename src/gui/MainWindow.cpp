#include "gui/Screens.h"
#include "gui/ThemeManager.h"
#include "gui/Components.h"
#include "core/AnimationManager.h"
#include "core/Localization.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace hat::gui {

namespace {

const char* kTabs[5] = {"Combat", "Visuals", "Movement", "Misc", "Settings"};
const char* kTabDesc[5] = {
    "UI demo only - nothing here touches any game.",
    "Preview canvas, crosshair and settings (everything is drawn inside Hatenium).",
    "Movement presets and indicators (demo only).",
    "Interface extras and notifications.",
    "Appearance, window, application and configuration."};

constexpr float kTwoPi = 6.2831853f;

// Small vector icons so no icon font is required.
void DrawTabIcon(ImDrawList* dl, int tab, ImVec2 c, float s, ImU32 col) {
    float lw = std::max(1.4f, s * 0.11f);
    switch (tab) {
    case 0:   // crosshair
        dl->AddCircle(c, s * 0.34f, col, 24, lw);
        dl->AddLine(ImVec2(c.x - s * 0.5f, c.y), ImVec2(c.x - s * 0.16f, c.y), col, lw);
        dl->AddLine(ImVec2(c.x + s * 0.16f, c.y), ImVec2(c.x + s * 0.5f, c.y), col, lw);
        dl->AddLine(ImVec2(c.x, c.y - s * 0.5f), ImVec2(c.x, c.y - s * 0.16f), col, lw);
        dl->AddLine(ImVec2(c.x, c.y + s * 0.16f), ImVec2(c.x, c.y + s * 0.5f), col, lw);
        break;
    case 1: { // eye
        ImVec2 l(c.x - s * 0.5f, c.y), r(c.x + s * 0.5f, c.y);
        dl->AddBezierCubic(l, ImVec2(c.x - s * 0.2f, c.y - s * 0.55f), ImVec2(c.x + s * 0.2f, c.y - s * 0.55f), r, col, lw);
        dl->AddBezierCubic(l, ImVec2(c.x - s * 0.2f, c.y + s * 0.55f), ImVec2(c.x + s * 0.2f, c.y + s * 0.55f), r, col, lw);
        dl->AddCircleFilled(c, s * 0.16f, col, 16);
        break; }
    case 2:   // double chevron
        for (int i = 0; i < 2; ++i) {
            float x = c.x - s * 0.2f + i * s * 0.34f;
            dl->AddLine(ImVec2(x - s * 0.14f, c.y - s * 0.3f), ImVec2(x + s * 0.14f, c.y), col, lw);
            dl->AddLine(ImVec2(x + s * 0.14f, c.y), ImVec2(x - s * 0.14f, c.y + s * 0.3f), col, lw);
        }
        break;
    case 3:   // 2x2 grid
        for (int i = 0; i < 4; ++i) {
            float x = c.x + ((i & 1) ? s * 0.08f : -s * 0.44f), y = c.y + ((i & 2) ? s * 0.08f : -s * 0.44f);
            dl->AddRect(ImVec2(x, y), ImVec2(x + s * 0.36f, y + s * 0.36f), col, s * 0.07f, 0, lw);
        }
        break;
    default: { // gear
        dl->AddCircle(c, s * 0.2f, col, 16, lw);
        dl->AddCircle(c, s * 0.38f, col, 24, lw);
        for (int i = 0; i < 8; ++i) {
            float a = i * kTwoPi / 8.f;
            dl->AddLine(ImVec2(c.x + std::cos(a) * s * 0.38f, c.y + std::sin(a) * s * 0.38f),
                        ImVec2(c.x + std::cos(a) * s * 0.52f, c.y + std::sin(a) * s * 0.52f), col, lw * 1.4f);
        }
        break; }
    }
}

// Header button (minimize / close) drawn manually.
bool HeaderButton(const char* id, ImVec2 size, bool isClose, const Theme& th) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    float hv = AnimationManager::Get().Smooth(ImGui::GetID("##hv"), hovered ? 1.f : 0.f, 16.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec4 hoverBg = isClose ? ImVec4(0.9f, 0.2f, 0.25f, 0.9f) : WithAlpha(th.accent, 0.18f);
    float r = std::min(th.buttonRounding, S(8));
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), C32(WithAlpha(hoverBg, hv)), r);
    ImVec2 c(p.x + size.x * 0.5f, p.y + size.y * 0.5f);
    float h = S(5);
    ImU32 col = C32(Mix(th.textDim, isClose ? ImVec4(1, 1, 1, 1) : th.text, hv));
    if (isClose) {
        dl->AddLine(ImVec2(c.x - h, c.y - h), ImVec2(c.x + h, c.y + h), col, 1.5f);
        dl->AddLine(ImVec2(c.x - h, c.y + h), ImVec2(c.x + h, c.y - h), col, 1.5f);
    } else {
        dl->AddLine(ImVec2(c.x - h, c.y + S(2)), ImVec2(c.x + h, c.y + S(2)), col, 1.5f);
    }
    return clicked;
}

void DrawAnimatedAccentLine(ImDrawList* dl, ImVec2 a, ImVec2 b, const Theme& th, float time) {
    dl->AddLine(a, b, C32(th.border));
    float w = b.x - a.x, seg = w * 0.32f;
    float x = a.x + std::fmod(time * 0.35f, 1.f) * (w + seg) - seg;
    float x0 = std::max(x, a.x), x1 = std::min(x + seg, b.x);
    if (x1 <= x0) return;
    float xm = (x + x + seg) * 0.5f;
    ImU32 clear = C32(WithAlpha(th.accent, 0.f)), full = C32(th.accent);
    if (xm > x0) dl->AddRectFilledMultiColor(ImVec2(x0, a.y - 1), ImVec2(std::min(xm, x1), a.y + 1), clear, full, full, clear);
    if (x1 > xm) dl->AddRectFilledMultiColor(ImVec2(std::max(xm, x0), a.y - 1), ImVec2(x1, a.y + 1), full, clear, clear, full);
}

} // namespace

void DrawMain(Application& app) {
    const Theme& th = ThemeManager::Get().T();
    AnimationManager& am = AnimationManager::Get();
    Settings& s = app.settings();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos(), size = ImGui::GetWindowSize();
    float headerH = S(46);
    bool compact = size.x < S(820);
    float sideW = compact ? S(62) : S(184);
    float R = th.windowRounding;

    // ------------------------------------------------------------ header
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + headerH), C32(th.panel), R, ImDrawFlags_RoundCornersTop);
    ImVec2 lineA(pos.x, pos.y + headerH), lineB(pos.x + size.x, pos.y + headerH);
    if (s.misc.animatedAccent) DrawAnimatedAccentLine(dl, lineA, lineB, th, am.Time());
    else dl->AddLine(lineA, lineB, C32(th.border));

    // logo + title
    ImVec2 lc(pos.x + S(24), pos.y + headerH * 0.5f);
    dl->AddCircle(lc, S(8), C32(th.accent), 32, S(2.2f));
    dl->AddText(ImVec2(pos.x + S(40), pos.y + (headerH - ImGui::GetFontSize()) * 0.5f), C32(th.text), "HATENIUM");
    float tw = ImGui::CalcTextSize("HATENIUM").x;
    char ver[24];
    std::snprintf(ver, sizeof(ver), "Version %s", HATENIUM_VERSION);
    if (!compact || size.x > S(600))
        dl->AddText(ImVec2(pos.x + S(40) + tw + S(10), pos.y + (headerH - ImGui::GetFontSize()) * 0.5f), C32(th.textMuted), ver);

    // right side: FPS, minimise, close
    ImVec2 bsz(S(32), S(28));
    float by = pos.y + (headerH - bsz.y) * 0.5f;
    float closeX = pos.x + size.x - bsz.x - S(12);
    float minX = closeX - bsz.x - S(4);
    float rightEdge = minX - S(8);
    if (s.misc.fpsCounter) {
        char fps[24];
        std::snprintf(fps, sizeof(fps), "%d FPS", static_cast<int>(app.Fps() + 0.5f));
        ImVec2 fs = ImGui::CalcTextSize(fps);
        dl->AddText(ImVec2(rightEdge - fs.x - S(8), pos.y + (headerH - fs.y) * 0.5f), C32(th.accent), fps);
        rightEdge -= fs.x + S(20);
    }
    // drag area (everything left of the buttons)
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("##drag", ImVec2(std::max(rightEdge - pos.x, 10.f), headerH));
    if (ImGui::IsItemActivated()) {
        app.window().BeginDragMove();
        ImGui::GetIO().AddMouseButtonEvent(0, false);   // the system move loop swallows the button-up
    }
    ImGui::SetCursorScreenPos(ImVec2(minX, by));
    if (HeaderButton("##min", bsz, false, th)) app.window().Minimize();
    ImGui::SetCursorScreenPos(ImVec2(closeX, by));
    if (HeaderButton("##close", bsz, true, th)) app.RequestClose();

    // ------------------------------------------------------------ sidebar
    ImVec2 sp0(pos.x, pos.y + headerH + 1), sp1(pos.x + sideW, pos.y + size.y);
    dl->AddRectFilled(sp0, sp1, C32(WithAlpha(th.panel, 0.65f)), R, ImDrawFlags_RoundCornersBottomLeft);
    dl->AddLine(ImVec2(sp1.x, sp0.y), ImVec2(sp1.x, sp1.y), C32(th.border));

    float itemH = S(42), itemGap = S(4), ix = pos.x + S(8), iw = sideW - S(16);
    float iy0 = pos.y + headerH + S(14);
    int active = std::min(std::max(s.lastTab, 0), 4);

    float selY = am.Smooth(ImGui::GetID("##selY"), active * (itemH + itemGap), 18.f);
    float rr = std::min(th.buttonRounding, S(10));
    dl->AddRectFilled(ImVec2(ix, iy0 + selY), ImVec2(ix + iw, iy0 + selY + itemH), C32(WithAlpha(th.accent, 0.14f)), rr);
    dl->AddRectFilled(ImVec2(ix, iy0 + selY + S(9)), ImVec2(ix + S(3), iy0 + selY + itemH - S(9)), C32(th.accent), S(2));

    for (int i = 0; i < 5; ++i) {
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(ImVec2(ix, iy0 + i * (itemH + itemGap)));
        bool clicked = ImGui::InvisibleButton("##tab", ImVec2(iw, itemH));
        bool hovered = ImGui::IsItemHovered();
        float hv = am.Smooth(ImGui::GetID("##th"), hovered ? 1.f : 0.f, 16.f);
        float act = am.Smooth(ImGui::GetID("##ta"), (i == active) ? 1.f : 0.f, 16.f);
        if (hovered && i != active) dl->AddRectFilled(ImVec2(ix, iy0 + i * (itemH + itemGap)), ImVec2(ix + iw, iy0 + i * (itemH + itemGap) + itemH), C32(WithAlpha(th.accent, 0.06f * hv)), rr);
        ImU32 col = C32(Mix(Mix(th.textMuted, th.textDim, hv), th.accent, act));
        ImVec2 ic(ix + (compact ? iw * 0.5f : S(24)), iy0 + i * (itemH + itemGap) + itemH * 0.5f);
        DrawTabIcon(dl, i, ic, S(17), col);
        if (!compact) {
            const char* name = tr(kTabs[i]);
            ImVec2 ts = ImGui::CalcTextSize(name);
            dl->AddText(ImVec2(ix + S(46), ic.y - ts.y * 0.5f), C32(Mix(Mix(th.textDim, th.text, hv), th.text, act)), name);
        }
        if (clicked && i != active) {
            s.lastTab = i;
            am.Channel("tab").Start(0.25f, 1.f, am.Duration(0.28f), Ease::OutCubic);
        }
        ImGui::PopID();
    }

    // status block at the bottom of the sidebar
    float sy = pos.y + size.y - S(112);
    dl->AddLine(ImVec2(ix, sy - S(8)), ImVec2(ix + iw, sy - S(8)), C32(th.border));
    float pulse = 0.5f + 0.5f * std::sin(am.Time() * 2.5f);
    ImVec2 dot(ix + S(6), sy + S(10));
    dl->AddCircleFilled(dot, S(5.5f), C32(WithAlpha(ImVec4(0.25f, 0.95f, 0.55f, 1), 0.18f + 0.12f * pulse)), 24);
    dl->AddCircleFilled(dot, S(3.2f), C32(ImVec4(0.25f, 0.95f, 0.55f, 1)), 16);
    if (!compact) {
        dl->AddText(ImVec2(ix + S(18), sy), C32(th.textMuted), tr("Current Tab"));
        dl->AddText(ImVec2(ix + S(18), sy + S(16)), C32(th.text), tr(kTabs[active]));
        dl->AddText(ImVec2(ix + S(18), sy + S(38)), C32(th.textMuted), tr("Status"));
        dl->AddText(ImVec2(ix + S(18), sy + S(54)), C32(ImVec4(0.45f, 0.95f, 0.65f, 1)), tr("Ready - UI demo"));
        char v[32];
        std::snprintf(v, sizeof(v), "%s %s", tr("Version"), HATENIUM_VERSION);
        dl->AddText(ImVec2(ix + S(0), sy + S(78)), C32(th.textMuted), v);
    } else {
        char v[16];
        std::snprintf(v, sizeof(v), "v%s", HATENIUM_VERSION);
        ImVec2 vs = ImGui::CalcTextSize(v);
        dl->AddText(ImVec2(ix + (iw - vs.x) * 0.5f, sy + S(30)), C32(th.textMuted), v);
    }

    // ------------------------------------------------------------ content
    ImGui::SetCursorScreenPos(ImVec2(pos.x + sideW + 1, pos.y + headerH + 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, S(18, 16));
    ImGui::BeginChild("##content", ImVec2(size.x - sideW - 1, size.y - headerH - 1),
                      ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();

    float tabA = am.Channel("tab").Value();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * std::min(std::max(tabA, 0.f), 1.f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (1.f - tabA) * S(16));
    ui::PageHeader(kTabs[active], kTabDesc[active]);
    switch (active) {
    case 0: DrawCombatPage(app); break;
    case 1: DrawVisualsPage(app); break;
    case 2: DrawMovementPage(app); break;
    case 3: DrawMiscPage(app); break;
    default: DrawSettingsPage(app); break;
    }
    ImGui::Dummy(ImVec2(0, S(12)));
    ImGui::PopStyleVar();
    ImGui::EndChild();

    // ------------------------------------------------------------ overlays
    if (s.misc.watermark) {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        char wm[64];
        std::snprintf(wm, sizeof(wm), "HATENIUM  |  v%s  |  %d fps", HATENIUM_VERSION, static_cast<int>(app.Fps() + 0.5f));
        ImVec2 ts = ImGui::CalcTextSize(wm);
        ImVec2 p1(pos.x + size.x - S(14), pos.y + size.y - S(14)), p0(p1.x - ts.x - S(20), p1.y - ts.y - S(12));
        fg->AddRectFilled(p0, p1, C32(WithAlpha(th.panel, 0.92f)), S(8));
        fg->AddRect(p0, p1, C32(th.borderBright), S(8));
        fg->AddRectFilled(ImVec2(p0.x + S(6), p0.y + S(6)), ImVec2(p0.x + S(8), p1.y - S(6)), C32(th.accent), 1.f);
        fg->AddText(ImVec2(p0.x + S(14), p0.y + S(6)), C32(th.textDim), wm);
    }
}

} // namespace hat::gui
