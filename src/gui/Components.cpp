#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/AnimationManager.h"
#include "core/Localization.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace hat::ui {

namespace {

AnimationManager& Anim() { return AnimationManager::Get(); }
const Theme& Th() { return ThemeManager::Get().T(); }

// Corner radius for small controls, following the chosen button shape.
float ControlRounding(float maxPx) { return std::min(Th().buttonRounding, maxPx); }

void DrawCheck(ImDrawList* dl, ImVec2 p, float size, float t, ImU32 col, float thick) {
    ImVec2 a(p.x + size * 0.24f, p.y + size * 0.52f);
    ImVec2 b(p.x + size * 0.43f, p.y + size * 0.72f);
    ImVec2 c(p.x + size * 0.78f, p.y + size * 0.30f);
    float t1 = std::min(t * 2.f, 1.f), t2 = std::max(t * 2.f - 1.f, 0.f);
    if (t1 <= 0.f) return;
    ImVec2 m(a.x + (b.x - a.x) * t1, a.y + (b.y - a.y) * t1);
    dl->AddLine(a, m, col, thick);
    if (t2 > 0.f) dl->AddLine(b, ImVec2(b.x + (c.x - b.x) * t2, b.y + (c.y - b.y) * t2), col, thick);
}

ImVec4 OnAccent(const ImVec4& a) {
    float lum = 0.299f * a.x + 0.587f * a.y + 0.114f * a.z;
    return lum > 0.55f ? ImVec4(0.02f, 0.04f, 0.07f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
}

// Shared slider implementation (float; ints are rounded).
bool SliderImpl(const char* label, float* v, float mn, float mx, const char* fmt, bool isInt) {
    const Theme& th = Th();
    ImGui::PushID(label);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x, h = S(38);
    ImGui::InvisibleButton("##slider", ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    ImGuiID id = ImGui::GetID("##a");

    float x0 = p.x + S(3), x1 = p.x + w - S(3), ty = p.y + h - S(11);
    bool changed = false;
    if (active) {
        float t = Clamp01((ImGui::GetIO().MousePos.x - x0) / std::max(x1 - x0, 1.f));
        float nv = mn + t * (mx - mn);
        if (isInt) nv = std::round(nv);
        if (nv != *v) { *v = nv; changed = true; }
    }
    float norm = Clamp01((*v - mn) / std::max(mx - mn, 0.0001f));
    float shown = Anim().Smooth(id, norm, 22.f);
    float hv = Anim().Smooth(ImGui::GetID("##h"), (hovered || active) ? 1.f : 0.f, 14.f);

    ImVec2 ts = ImGui::CalcTextSize(tr(label));
    dl->AddText(ImVec2(p.x, p.y + S(2)), C32(Mix(th.textDim, th.text, hv)), tr(label));
    char buf[48];
    if (isInt) std::snprintf(buf, sizeof(buf), fmt, static_cast<int>(*v));
    else       std::snprintf(buf, sizeof(buf), fmt, *v);
    ImVec2 vs = ImGui::CalcTextSize(buf);
    dl->AddText(ImVec2(p.x + w - vs.x, p.y + S(2)), C32(Mix(th.textMuted, th.accent, 0.5f + 0.5f * hv)), buf);
    (void)ts;

    float tr_ = S(4);
    dl->AddRectFilled(ImVec2(x0, ty - tr_ * 0.5f), ImVec2(x1, ty + tr_ * 0.5f), C32(th.frame), tr_);
    float fx = x0 + (x1 - x0) * shown;
    dl->AddRectFilled(ImVec2(x0, ty - tr_ * 0.5f), ImVec2(fx, ty + tr_ * 0.5f), C32(WithAlpha(th.accent, 0.55f + 0.45f * hv)), tr_);
    float gw = S(8) + S(2) * hv, gh = S(14) + S(3) * hv;
    float gr = ControlRounding(S(4));
    dl->AddRectFilled(ImVec2(fx - gw * 0.5f, ty - gh * 0.5f), ImVec2(fx + gw * 0.5f, ty + gh * 0.5f), C32(th.accent), gr);
    ImGui::PopID();
    return changed;
}

// ---- notifications ----
struct Note { std::string text; int kind; float age; };
std::vector<Note> g_notes;
bool g_notesEnabled = true;
constexpr float kNoteLife = 3.2f;

} // namespace

// ============================================================ controls
bool Checkbox(const char* label, bool* v) {
    const Theme& th = Th();
    ImGui::PushID(label);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x, h = S(28);
    bool clicked = ImGui::InvisibleButton("##cb", ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();
    if (clicked) *v = !*v;
    float t = Anim().Smooth(ImGui::GetID("##t"), *v ? 1.f : 0.f, 16.f);
    float hv = Anim().Smooth(ImGui::GetID("##h"), hovered ? 1.f : 0.f, 14.f);

    const char* text = tr(label);
    ImVec2 ts = ImGui::CalcTextSize(text);
    dl->AddText(ImVec2(p.x, p.y + (h - ts.y) * 0.5f), C32(Mix(th.textDim, th.text, std::max(t, hv * 0.7f))), text);

    float bs = S(18), r = ControlRounding(S(5));
    ImVec2 b0(p.x + w - bs - S(2), p.y + (h - bs) * 0.5f), b1(b0.x + bs, b0.y + bs);
    dl->AddRectFilled(b0, b1, C32(Mix(th.frame, th.accent, t)), r);
    dl->AddRect(b0, b1, C32(Mix(Mix(th.borderBright, th.accent, hv * 0.6f), th.accent, t)), r, 0, 1.f);
    DrawCheck(dl, b0, bs, t, C32(OnAccent(th.accent)), std::max(1.5f, S(2)));
    ImGui::PopID();
    return clicked;
}

bool SliderFloat(const char* label, float* v, float mn, float mx, const char* fmt) {
    return SliderImpl(label, v, mn, mx, fmt, false);
}
bool SliderInt(const char* label, int* v, int mn, int mx, const char* fmt) {
    float f = static_cast<float>(*v);
    bool c = SliderImpl(label, &f, static_cast<float>(mn), static_cast<float>(mx), fmt, true);
    if (c) *v = static_cast<int>(f);
    return c;
}

bool Combo(const char* label, int* cur, const char* const* items, int count) {
    const Theme& th = Th();
    ImGui::PushID(label);
    float w = ImGui::GetContentRegionAvail().x;
    float x0 = ImGui::GetCursorPosX();
    float cw = std::min(S(170), w * 0.62f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(th.textDim, "%s", tr(label));
    ImGui::SameLine(x0 + w - cw, 0.f);
    ImGui::SetNextItemWidth(cw);
    bool changed = false;
    *cur = std::min(std::max(*cur, 0), count - 1);
    if (ImGui::BeginCombo("##combo", tr(items[*cur]))) {
        for (int i = 0; i < count; ++i) {
            bool sel = (i == *cur);
            if (ImGui::Selectable(tr(items[i]), sel)) { *cur = i; changed = true; }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return changed;
}

bool ColorField(const char* label, std::array<float, 3>& col) {
    const Theme& th = Th();
    ImGui::PushID(label);
    float w = ImGui::GetContentRegionAvail().x;
    float x0 = ImGui::GetCursorPosX();
    float fh = ImGui::GetFrameHeight();
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(th.textDim, "%s", tr(label));
    ImGui::SameLine(x0 + w - fh * 1.6f, 0.f);
    bool c = ImGui::ColorEdit3("##c", col.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::PopID();
    return c;
}

static const char* KeyName(int k) {
    static char buf[24];
    if (k >= 1000) { std::snprintf(buf, sizeof(buf), "Mouse %d", k - 1000 + 1); return buf; }
    return ImGui::GetKeyName(static_cast<ImGuiKey>(k));
}

bool Keybind(const char* label, int* key) {
    const Theme& th = Th();
    static ImGuiID listening = 0;
    ImGui::PushID(label);
    ImGuiID id = ImGui::GetID("##kb");
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = ImGui::GetContentRegionAvail().x;
    float x0 = ImGui::GetCursorPosX();
    float bw = S(112), bh = S(26);
    float rowH = ImGui::GetFrameHeight();

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(th.textDim, "%s", tr(label));
    ImGui::SameLine(x0 + w - bw, 0.f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    p.y += (rowH - bh) * 0.5f;
    ImGui::SetCursorScreenPos(p);
    bool clicked = ImGui::InvisibleButton("##kb", ImVec2(bw, bh));
    bool hovered = ImGui::IsItemHovered();
    if (clicked) listening = (listening == id) ? 0 : id;

    bool changed = false;
    bool lis = (listening == id);
    if (lis) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { *key = 0; changed = true; listening = 0; }
        else if (ImGui::IsMouseClicked(0) && !hovered) { listening = 0; }
        else {
            for (int b = 1; b <= 4 && lis; ++b)
                if (ImGui::IsMouseClicked(b)) { *key = 1000 + b; changed = true; listening = 0; lis = false; }
            for (int k = ImGuiKey_Tab; k <= ImGuiKey_AppForward && lis; ++k)
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false)) { *key = k; changed = true; listening = 0; lis = false; }
        }
    }
    lis = (listening == id);
    float hv = Anim().Smooth(ImGui::GetID("##h"), (hovered || lis) ? 1.f : 0.f, 14.f);
    float r = ControlRounding(S(6));
    dl->AddRectFilled(p, ImVec2(p.x + bw, p.y + bh), C32(Mix(th.frame, th.cardHover, hv)), r);
    dl->AddRect(p, ImVec2(p.x + bw, p.y + bh), C32(Mix(th.border, th.accent, lis ? 1.f : hv * 0.5f)), r);
    const char* txt = lis ? "..." : (*key == 0 ? tr("None") : KeyName(*key));
    ImVec2 ts = ImGui::CalcTextSize(txt);
    dl->AddText(ImVec2(p.x + (bw - ts.x) * 0.5f, p.y + (bh - ts.y) * 0.5f),
                C32(lis ? th.accent : (*key == 0 ? th.textMuted : th.text)), txt);
    ImGui::PopID();
    return changed;
}

bool Button(const char* label, ImVec2 size, bool primary) {
    const Theme& th = Th();
    ImGui::PushID(label);
    const char* text = tr(label);
    ImVec2 ts = ImGui::CalcTextSize(text);
    if (size.x <= 0) size.x = ts.x + S(28);
    if (size.y <= 0) size.y = S(32);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton("##btn", size);
    bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    float hv = Anim().Smooth(ImGui::GetID("##h"), hovered ? 1.f : 0.f, 16.f);
    float av = Anim().Smooth(ImGui::GetID("##a"), active ? 1.f : 0.f, 24.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 q(p.x + size.x, p.y + size.y);
    float r = th.buttonRounding;
    if (primary) {
        ImVec4 base = Mix(th.accent, ImVec4(1, 1, 1, 1), 0.12f * hv);
        base = Mix(base, ImVec4(0, 0, 0, 1), 0.15f * av);
        dl->AddRectFilled(ImVec2(p.x - 1, p.y - 1), ImVec2(q.x + 1, q.y + 1), C32(WithAlpha(th.accent, 0.10f + 0.18f * hv)), r + 2);
        dl->AddRectFilled(p, q, C32(base), r);
        dl->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f), C32(OnAccent(th.accent)), text);
    } else {
        dl->AddRectFilled(p, q, C32(Mix(th.cardHover, Mix(th.cardHover, th.accent, 0.22f), hv)), r);
        dl->AddRect(p, q, C32(Mix(th.border, th.accent, hv)), r);
        dl->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f), C32(Mix(th.text, th.accent, hv * 0.6f)), text);
    }
    ImGui::PopID();
    return clicked;
}

bool ShapeTile(const char* id, const char* label, bool selected, float rounding, int kind, float width) {
    const Theme& th = Th();
    ImGui::PushID(id);
    float h = S(66);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton("##tile", ImVec2(width, h));
    bool hovered = ImGui::IsItemHovered();
    float sel = Anim().Smooth(ImGui::GetID("##s"), selected ? 1.f : 0.f, 16.f);
    float hv = Anim().Smooth(ImGui::GetID("##h"), hovered ? 1.f : 0.f, 14.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 q(p.x + width, p.y + h);
    float tr0 = std::min(S(8), th.cardRounding);
    dl->AddRectFilled(p, q, C32(Mix(th.frame, th.cardHover, hv)), tr0);
    dl->AddRect(p, q, C32(Mix(th.border, th.accent, std::max(sel, hv * 0.4f))), tr0, 0, 1.f + sel);

    ImVec2 c(p.x + width * 0.5f, p.y + S(23));
    float rr = rounding * 0.55f * th.scale;
    if (kind == 0) {
        float bw = std::min(width * 0.62f, S(54)), bh = S(18);
        dl->AddRectFilled(ImVec2(c.x - bw * 0.5f, c.y - bh * 0.5f), ImVec2(c.x + bw * 0.5f, c.y + bh * 0.5f), C32(th.accent), rr);
    } else {
        float bw = std::min(width * 0.50f, S(40)), bh = S(26);
        ImVec2 a(c.x - bw * 0.5f, c.y - bh * 0.5f), b(c.x + bw * 0.5f, c.y + bh * 0.5f);
        dl->AddRectFilled(a, b, C32(th.panel), rr);
        dl->AddRect(a, b, C32(th.accent), rr, 0, 1.5f);
        dl->AddLine(ImVec2(a.x + S(5), a.y + S(8)), ImVec2(b.x - S(5), a.y + S(8)), C32(WithAlpha(th.accent, 0.6f)), 1.f);
    }
    const char* text = tr(label);
    ImVec2 ts = ImGui::CalcTextSize(text);
    dl->AddText(ImVec2(p.x + (width - ts.x) * 0.5f, p.y + h - ts.y - S(7)), C32(Mix(th.textDim, th.text, std::max(sel, hv))), text);
    ImGui::PopID();
    return clicked;
}

bool SeekBar(const char* id, float frac, float* outFrac) {
    const Theme& th = Th();
    static ImGuiID dragging = 0;
    static float dragFrac = 0.f;
    ImGui::PushID(id);
    ImGuiID gid = ImGui::GetID("##seek");
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x, h = S(20);
    ImGui::InvisibleButton("##seek", ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    float x0 = p.x + S(3), x1 = p.x + w - S(3), cy = p.y + h * 0.5f;
    bool released = false;
    if (active) { dragging = gid; dragFrac = Clamp01((ImGui::GetIO().MousePos.x - x0) / std::max(x1 - x0, 1.f)); }
    if (ImGui::IsItemDeactivated() && dragging == gid) { *outFrac = dragFrac; dragging = 0; released = true; }
    float shown = (dragging == gid) ? dragFrac : Clamp01(frac);
    float hv = Anim().Smooth(ImGui::GetID("##h"), (hovered || active) ? 1.f : 0.f, 14.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float tr_ = S(5);
    dl->AddRectFilled(ImVec2(x0, cy - tr_ * 0.5f), ImVec2(x1, cy + tr_ * 0.5f), C32(th.frame), tr_);
    float fx = x0 + (x1 - x0) * shown;
    dl->AddRectFilled(ImVec2(x0, cy - tr_ * 0.5f), ImVec2(fx, cy + tr_ * 0.5f), C32(th.accent), tr_);
    dl->AddCircleFilled(ImVec2(fx, cy), S(5.f) + S(2.f) * hv, C32(th.accent), 20);
    ImGui::PopID();
    return released;
}

bool Swatch(const char* id, const ImVec4& color, bool selected, float d) {
    const Theme& th = Th();
    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton("##sw", ImVec2(d, d));
    bool hovered = ImGui::IsItemHovered();
    float sel = Anim().Smooth(ImGui::GetID("##s"), selected ? 1.f : 0.f, 16.f);
    float hv = Anim().Smooth(ImGui::GetID("##h"), hovered ? 1.f : 0.f, 14.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 c(p.x + d * 0.5f, p.y + d * 0.5f);
    float r = d * 0.5f - S(5) + S(2) * hv;
    dl->AddCircleFilled(c, r, C32(color), 32);
    if (sel > 0.01f) dl->AddCircle(c, d * 0.5f - S(1.5f), C32(WithAlpha(color, sel)), 32, S(2));
    (void)th;
    ImGui::PopID();
    return clicked;
}

// ============================================================ layout
namespace {
// "Masonry" layout: every card goes into the currently shortest column, so columns stay balanced and no gaps remain.
struct Masonry {
    bool active = false;
    int cols = 1, cur = 0;
    float x0 = 0.f, y0 = 0.f, colW = 0.f, gapX = 0.f, gapY = 0.f;
    float colY[4] = {0.f, 0.f, 0.f, 0.f};
} g_m;
} // namespace

bool BeginCard(const char* title) {
    const Theme& th = Th();
    ImVec2 size(0, 0);
    if (g_m.active) {
        int c = 0;
        for (int i = 1; i < g_m.cols; ++i) if (g_m.colY[i] < g_m.colY[c] - 0.5f) c = i;
        g_m.cur = c;
        ImGui::SetCursorPos(ImVec2(g_m.x0 + c * (g_m.colW + g_m.gapX), g_m.y0 + g_m.colY[c]));
        size = ImVec2(g_m.colW, 0);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, S(14, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, th.cardRounding);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, WithAlpha(th.card, th.panelOpacity));
    ImGui::BeginChild(title, size,
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(ImVec2(p.x, p.y + S(2)), ImVec2(p.x + S(3), p.y + S(16)), C32(th.accent), S(2));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(11));
    ImGui::TextColored(th.text, "%s", tr(title));
    ImGui::Dummy(ImVec2(0, S(2)));
    return true;
}

void EndCard() {
    ImGui::EndChild();
    if (g_m.active) {
        float h = ImGui::GetItemRectSize().y;
        if (h < 1.f) h = S(180);   // first frame of a card: ImGui has not measured it yet
        g_m.colY[g_m.cur] += h + g_m.gapY;
    }
}

bool BeginColumns(const char*, int count) {
    g_m = Masonry();
    g_m.active = true;
    g_m.cols = std::min(std::max(count, 1), 4);
    const ImGuiStyle& st = ImGui::GetStyle();
    g_m.gapX = st.ItemSpacing.x * 1.0f;
    g_m.gapY = st.ItemSpacing.y;
    float avail = ImGui::GetContentRegionAvail().x;
    g_m.colW = std::floor((avail - g_m.gapX * (g_m.cols - 1)) / g_m.cols);
    g_m.x0 = ImGui::GetCursorPosX();
    g_m.y0 = ImGui::GetCursorPosY();
    return true;
}

void EndColumns() {
    float maxY = 0.f;
    for (int i = 0; i < g_m.cols; ++i) maxY = std::max(maxY, g_m.colY[i]);
    ImGui::SetCursorPos(ImVec2(g_m.x0, g_m.y0 + std::max(maxY - g_m.gapY, 0.f)));
    ImGui::Dummy(ImVec2(1.f, 1.f));
    g_m.active = false;
}

int ColumnCountForWidth(float minColumnLogical) {
    float w = ImGui::GetContentRegionAvail().x;
    return std::max(1, std::min(3, static_cast<int>(w / S(minColumnLogical))));
}

void PageHeader(const char* title, const char* description) {
    const Theme& th = Th();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* f = ImGui::GetFont();
    float size = f->FontSize * 1.45f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    dl->AddText(f, size, p, C32(th.text), tr(title));
    ImGui::Dummy(ImVec2(0, size + S(2)));
    ImGui::TextColored(th.textMuted, "%s", tr(description));
    ImGui::Dummy(ImVec2(0, S(4)));
}

void NoteText(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, Th().textMuted);
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextUnformatted(tr(text));
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
}

// ============================================================ notifications
void SetNotificationsEnabled(bool on) { g_notesEnabled = on; if (!on) g_notes.clear(); }

void Notify(const std::string& text, int kind) {
    if (!g_notesEnabled) return;
    if (g_notes.size() >= 5) g_notes.erase(g_notes.begin());
    g_notes.push_back({text, kind, 0.f});
}

void DrawNotifications(ImVec2 topRight) {
    if (g_notes.empty()) return;
    const Theme& th = Th();
    float dt = ImGui::GetIO().DeltaTime;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    float gap = S(8);
    float maxW = std::max(S(200), ImGui::GetMainViewport()->Size.x - S(28));
    float wrapW = std::min(S(380), maxW) - S(34);
    float y = topRight.y;
    for (auto& n : g_notes) n.age += dt;
    g_notes.erase(std::remove_if(g_notes.begin(), g_notes.end(), [](const Note& n) { return n.age > kNoteLife; }), g_notes.end());
    for (const auto& n : g_notes) {
        ImVec2 ts = ImGui::CalcTextSize(n.text.c_str(), nullptr, false, wrapW);
        float w = std::max(S(220), ts.x + S(34)), h = std::max(S(40), ts.y + S(20));
        float in = ApplyEase(Ease::OutCubic, Clamp01(n.age / 0.28f));
        float out = Clamp01((kNoteLife - n.age) / 0.35f);
        float a = in * out;
        float x = topRight.x - w + (1.f - in) * (w + S(24));
        ImVec4 bar = n.kind == 1 ? ImVec4(0.25f, 0.95f, 0.55f, 1) : (n.kind == 2 ? ImVec4(1.f, 0.75f, 0.2f, 1) : th.accent);
        float r = std::min(th.cardRounding, S(10));
        ImVec2 p0(x, y), p1(x + w, y + h);
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(WithAlpha(th.panel, 0.97f * a)), r);
        dl->AddRect(p0, p1, ImGui::GetColorU32(WithAlpha(th.borderBright, a)), r);
        dl->AddRectFilled(ImVec2(x + S(8), y + S(9)), ImVec2(x + S(11), y + h - S(9)), ImGui::GetColorU32(WithAlpha(bar, a)), S(2));
        dl->AddText(nullptr, 0.f, ImVec2(x + S(20), y + (h - ts.y) * 0.5f), ImGui::GetColorU32(WithAlpha(th.text, a)),
                    n.text.c_str(), nullptr, wrapW);
        y += (h + gap) * in;
    }
}

} // namespace hat::ui
