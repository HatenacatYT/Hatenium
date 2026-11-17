#include "rendering/OverlayWindow.h"
#include "core/Utf.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <objidl.h>
namespace Gdiplus { using std::min; using std::max; }
#include <gdiplus.h>

namespace hat {

namespace {
const wchar_t* kOverlayClass = L"HateniumOverlayWindow";

bool g_overlayEditing = false;
int g_wheelAccum = 0;          // mouse-wheel delta received while the layout editor is active

LRESULT CALLBACK OverlayProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_NCHITTEST) return g_overlayEditing ? HTCLIENT : HTTRANSPARENT;   // normally mouse input goes to the window below
    if (m == WM_MOUSEWHEEL && g_overlayEditing) { g_wheelAccum += GET_WHEEL_DELTA_WPARAM(w); return 0; }
    if (m == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    return DefWindowProcW(h, m, w, l);
}

struct MonitorList { std::vector<RECT> rects; RECT primary{}; bool havePrimary = false; };

BOOL CALLBACK EnumMon(HMONITOR m, HDC, LPRECT, LPARAM lp) {
    auto* list = reinterpret_cast<MonitorList*>(lp);
    MONITORINFO mi{}; mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(m, &mi)) {
        if (mi.dwFlags & MONITORINFOF_PRIMARY) { list->primary = mi.rcMonitor; list->havePrimary = true; }
        else list->rects.push_back(mi.rcMonitor);
    }
    return TRUE;
}

MonitorList GetMonitors() {
    MonitorList l;
    EnumDisplayMonitors(nullptr, nullptr, EnumMon, reinterpret_cast<LPARAM>(&l));
    if (!l.havePrimary) { l.primary = RECT{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)}; l.havePrimary = true; }
    return l;
}

// Grows a rectangle so it contains the given box.
void Touch(RECT& r, bool& any, float x0, float y0, float x1, float y1) {
    RECT b{static_cast<LONG>(std::floor(x0)) - 2, static_cast<LONG>(std::floor(y0)) - 2,
           static_cast<LONG>(std::ceil(x1)) + 2, static_cast<LONG>(std::ceil(y1)) + 2};
    if (!any) { r = b; any = true; return; }
    r.left = std::min(r.left, b.left); r.top = std::min(r.top, b.top);
    r.right = std::max(r.right, b.right); r.bottom = std::max(r.bottom, b.bottom);
}

Gdiplus::Color ToColor(const Rgba& c) {
    auto B = [](float v) { return static_cast<BYTE>(std::min(std::max(v, 0.f), 1.f) * 255.f + 0.5f); };
    return Gdiplus::Color(B(c.a), B(c.r), B(c.g), B(c.b));
}

class GdiSink : public DrawSink {
public:
    GdiSink(Gdiplus::Graphics* g, RECT* dirty, bool* any) : g_(g), dirty_(dirty), any_(any) {}
    void Line(Vec2 a, Vec2 b, float th, Rgba c) override {
        Gdiplus::Pen pen(ToColor(c), th);
        pen.SetStartCap(Gdiplus::LineCapRound); pen.SetEndCap(Gdiplus::LineCapRound);
        g_->DrawLine(&pen, a.x, a.y, b.x, b.y);
        Touch(*dirty_, *any_, std::min(a.x, b.x) - th, std::min(a.y, b.y) - th, std::max(a.x, b.x) + th, std::max(a.y, b.y) + th);
    }
    void Circle(Vec2 p, float r, Rgba c, bool filled, float th) override {
        if (filled) { Gdiplus::SolidBrush br(ToColor(c)); g_->FillEllipse(&br, p.x - r, p.y - r, 2 * r, 2 * r); }
        else { Gdiplus::Pen pen(ToColor(c), th); g_->DrawEllipse(&pen, p.x - r, p.y - r, 2 * r, 2 * r); }
        float e = r + th;
        Touch(*dirty_, *any_, p.x - e, p.y - e, p.x + e, p.y + e);
    }
    void Polygon(const Vec2* pts, int n, Rgba c, bool filled, float th) override {
        Gdiplus::PointF q[16];
        n = std::min(n, 16);
        float x0 = pts[0].x, y0 = pts[0].y, x1 = x0, y1 = y0;
        for (int i = 0; i < n; ++i) {
            q[i] = Gdiplus::PointF(pts[i].x, pts[i].y);
            x0 = std::min(x0, pts[i].x); y0 = std::min(y0, pts[i].y); x1 = std::max(x1, pts[i].x); y1 = std::max(y1, pts[i].y);
        }
        if (filled) { Gdiplus::SolidBrush br(ToColor(c)); g_->FillPolygon(&br, q, n); }
        else { Gdiplus::Pen pen(ToColor(c), th); g_->DrawPolygon(&pen, q, n); }
        Touch(*dirty_, *any_, x0 - th, y0 - th, x1 + th, y1 + th);
    }
    void RoundRect(Vec2 a, Vec2 b, float r, Rgba fill, Rgba border, float bt) override {
        float w = b.x - a.x, h = b.y - a.y;
        r = std::min(r, std::min(w, h) * 0.5f);
        Gdiplus::GraphicsPath path;
        float d = r * 2.f;
        path.AddArc(a.x, a.y, d, d, 180, 90);
        path.AddArc(b.x - d, a.y, d, d, 270, 90);
        path.AddArc(b.x - d, b.y - d, d, d, 0, 90);
        path.AddArc(a.x, b.y - d, d, d, 90, 90);
        path.CloseFigure();
        Gdiplus::SolidBrush br(ToColor(fill));
        g_->FillPath(&br, &path);
        if (bt > 0.f) { Gdiplus::Pen pen(ToColor(border), bt); g_->DrawPath(&pen, &path); }
        Touch(*dirty_, *any_, a.x - bt, a.y - bt, b.x + bt, b.y + bt);
    }
    void Text(Vec2 p, const char* utf8, float px, Rgba c, int align, bool bold) override {
        std::wstring w = WideFromUtf8(utf8);
        Gdiplus::FontFamily fam(L"Segoe UI");
        const Gdiplus::FontFamily* use = fam.IsAvailable() ? &fam : Gdiplus::FontFamily::GenericSansSerif();
        Gdiplus::Font font(use, px, bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::StringFormat sf;
        sf.SetAlignment(align == 1 ? Gdiplus::StringAlignmentCenter : Gdiplus::StringAlignmentNear);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        Gdiplus::SolidBrush br(ToColor(c));
        float bw = std::max(400.f, static_cast<float>(w.size()) * px * 0.9f), bh = px * 3.f;
        float x0 = align == 1 ? p.x - bw * 0.5f : p.x;
        g_->DrawString(w.c_str(), -1, &font, Gdiplus::RectF(x0, p.y - bh * 0.5f, bw, bh), &sf, &br);
        float est = static_cast<float>(w.size()) * px * 0.75f + 4.f;
        float tx0 = align == 1 ? p.x - est * 0.5f : p.x;
        Touch(*dirty_, *any_, tx0 - 2.f, p.y - px, tx0 + est + 2.f, p.y + px);
    }
private:
    Gdiplus::Graphics* g_;
    RECT* dirty_;
    bool* any_;
};
} // namespace

int OverlayWindow::MonitorCount() { return static_cast<int>(GetMonitors().rects.size()) + 1; }
int OverlayWindow::PrimaryScreenHeight() { RECT p = GetMonitors().primary; return std::max<int>(1, p.bottom - p.top); }

bool OverlayWindow::Init(HINSTANCE inst) {
    inst_ = inst;
    Gdiplus::GdiplusStartupInput in;
    gdiOk_ = Gdiplus::GdiplusStartup(&gdiToken_, &in, nullptr) == Gdiplus::Ok;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kOverlayClass;
    RegisterClassExW(&wc);
    return gdiOk_;
}

void OverlayWindow::Shutdown() {
    DestroyWindowAndBuffers();
    if (gdiOk_) { Gdiplus::GdiplusShutdown(gdiToken_); gdiOk_ = false; }
    UnregisterClassW(kOverlayClass, inst_);
}

void OverlayWindow::DestroyWindowAndBuffers() {
    delete wmImg_; wmImg_ = nullptr; wmPath_.clear();
    delete g_; g_ = nullptr;
    delete bmp_; bmp_ = nullptr;
    if (memDC_) {
        if (oldBmp_) SelectObject(memDC_, oldBmp_);
        DeleteDC(memDC_); memDC_ = nullptr;
    }
    if (dib_) { DeleteObject(dib_); dib_ = nullptr; }
    oldBmp_ = nullptr; bits_ = nullptr; w_ = h_ = 0;
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    needFull_ = true;
    staticKey_.clear();
    wasActive_ = false;
    prevDirty_ = RECT{};
}

void OverlayWindow::CreateBuffers(int w, int h) {
    delete g_; g_ = nullptr;
    delete bmp_; bmp_ = nullptr;
    if (memDC_) { if (oldBmp_) SelectObject(memDC_, oldBmp_); }
    if (dib_) { DeleteObject(dib_); dib_ = nullptr; }
    if (!memDC_) memDC_ = CreateCompatibleDC(nullptr);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;               // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    dib_ = CreateDIBSection(memDC_, &bi, DIB_RGB_COLORS, &bits_, nullptr, 0);
    oldBmp_ = static_cast<HBITMAP>(SelectObject(memDC_, dib_));
    w_ = w; h_ = h;
    bmp_ = new Gdiplus::Bitmap(w, h, w * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(bits_));
    g_ = new Gdiplus::Graphics(bmp_);
    g_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g_->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    g_->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    needFull_ = true;
}

bool OverlayWindow::EnsureWindow(const RECT& r) {
    int w = r.right - r.left, h = r.bottom - r.top;
    if (w <= 0 || h <= 0) return false;
    if (!hwnd_) {
        hwnd_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                kOverlayClass, L"Hatenium Overlay", WS_POPUP, r.left, r.top, w, h,
                                nullptr, nullptr, inst_, nullptr);
        if (!hwnd_) return false;
        rect_ = RECT{0, 0, 0, 0};
    }
    if (w != w_ || h != h_ || !bits_) CreateBuffers(w, h);
    if (r.left != rect_.left || r.top != rect_.top || r.right != rect_.right || r.bottom != rect_.bottom) {
        rect_ = r;
        SetWindowPos(hwnd_, HWND_TOPMOST, r.left, r.top, w, h, SWP_NOACTIVATE);
        needFull_ = true;
    }
    return bits_ != nullptr;
}

void OverlayWindow::SetInteractive(bool on) {
    editing_ = on;
    g_overlayEditing = on;
    if (!hwnd_) return;
    LONG_PTR ex = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    ex = on ? (ex & ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT)) : (ex | WS_EX_TRANSPARENT);
    SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, ex);
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    needFull_ = true;
    dragId_ = -1; prevLbEdit_ = true; guide_ = Vec2{-1.f, -1.f};
}

void OverlayWindow::Update(float dt, Settings& s, bool& editMode, const std::string& timerText) {
    const VisualSettings& v = s.visuals;
    if (!v.overlay.enable || !gdiOk_) {
        if (hwnd_) DestroyWindowAndBuffers();
        engine_.Clear();
        editing_ = g_overlayEditing = false;
        editMode = false;
        return;
    }

    // ---- target monitor ----
    MonitorList mons = GetMonitors();
    RECT target = mons.primary, anchor = mons.primary;
    if (v.overlay.monitor == 100) {
        target = RECT{GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), 0, 0};
        target.right = target.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
        target.bottom = target.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    } else if (v.overlay.monitor > 0 && !mons.rects.empty()) {
        size_t i = std::min<size_t>(static_cast<size_t>(v.overlay.monitor - 1), mons.rects.size() - 1);
        target = anchor = mons.rects[i];
    }
    if (!EnsureWindow(target)) return;
    {
        std::string k = std::to_string(anchor.right - anchor.left) + "x" + std::to_string(anchor.bottom - anchor.top);
        if (k != monKey_) { monKey_ = k; monKeyChanged_ = true; }
    }
    if (editMode != editing_) SetInteractive(editMode);

    // ---- input state (read-only polling) ----
    POINT cp{};
    GetCursorPos(&cp);
    bool inside = PtInRect(&rect_, cp) != 0;
    Vec2 cur{static_cast<float>(cp.x - rect_.left), static_cast<float>(cp.y - rect_.top)};
    bool lb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0, rb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    engine_.Update(dt, cur, inside, lb, rb, v.trail, v.particles, 1.f);
    hud_.Update(dt, s.hud);

    // ---- watermark image (GDI+ decodes PNG / JPEG) ----
    {
        std::wstring wp = WideFromUtf8(s.hud.watermark.path);
        if (wp != wmPath_) {
            delete wmImg_; wmImg_ = nullptr; wmPath_ = wp;
            if (!wp.empty()) {
                wmImg_ = Gdiplus::Image::FromFile(wp.c_str());
                if (wmImg_ && wmImg_->GetLastStatus() != Gdiplus::Ok) { delete wmImg_; wmImg_ = nullptr; }
            }
            needFull_ = true;
        }
    }
    float aw = static_cast<float>(anchor.right - anchor.left), ah = static_cast<float>(anchor.bottom - anchor.top);
    Vec2 aorg{static_cast<float>(anchor.left - rect_.left), static_cast<float>(anchor.top - rect_.top)};
    RECT wmRect{0, 0, 0, 0};
    bool hasWm = s.hud.watermark.enable && wmImg_ != nullptr;
    if (hasWm) {
        float iw = static_cast<float>(wmImg_->GetWidth()), ih = static_cast<float>(wmImg_->GetHeight());
        float w = std::min(iw * s.hud.watermark.scale, aw * 0.6f), h = w * ih / std::max(iw, 1.f);
        float x = aorg.x + s.hud.watermark.posX * 0.01f * aw, y = aorg.y + s.hud.watermark.posY * 0.01f * ah;
        wmRect = RECT{static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(x + w), static_cast<LONG>(y + h)};
    }

    // ---- layout editor (drag elements, snap to edges / centre) ----
    std::vector<HudRenderer::Box> boxes;
    if (editing_) {
        boxes = hud_.Bounds(s.hud, aorg, Vec2{aw, ah});
        if (hasWm) boxes.push_back({3, Vec2{static_cast<float>(wmRect.left), static_cast<float>(wmRect.top)}, Vec2{static_cast<float>(wmRect.right), static_cast<float>(wmRect.bottom)}});
        bool lbEdge = lb && !prevLbEdit_;
        prevLbEdit_ = lb;
        bool esc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        float dbx = aorg.x + aw * 0.5f - 60.f, dby = aorg.y + 14.f;
        bool onDone = cur.x >= dbx && cur.x <= dbx + 120.f && cur.y >= dby && cur.y <= dby + 34.f;
        if (esc || (lbEdge && onDone)) { editMode = false; }
        else if (lbEdge && dragId_ < 0) {
            for (int i = static_cast<int>(boxes.size()) - 1; i >= 0; --i)
                if (cur.x >= boxes[static_cast<size_t>(i)].a.x && cur.x <= boxes[static_cast<size_t>(i)].b.x &&
                    cur.y >= boxes[static_cast<size_t>(i)].a.y && cur.y <= boxes[static_cast<size_t>(i)].b.y) {
                    dragId_ = boxes[static_cast<size_t>(i)].id;
                    grab_ = Vec2{cur.x - boxes[static_cast<size_t>(i)].a.x, cur.y - boxes[static_cast<size_t>(i)].a.y};
                    break;
                }
        }
        // ---- Ctrl + mouse wheel (or Ctrl + '+' / '-') resizes the element under the cursor ----
        {
            bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            int wheel = g_wheelAccum;
            g_wheelAccum = 0;
            static bool prevPlus = false, prevMinus = false;
            bool plus = (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) || (GetAsyncKeyState(VK_ADD) & 0x8000);
            bool minus = (GetAsyncKeyState(VK_OEM_MINUS) & 0x8000) || (GetAsyncKeyState(VK_SUBTRACT) & 0x8000);
            if (ctrl) {
                if (plus && !prevPlus) wheel += 120;
                if (minus && !prevMinus) wheel -= 120;
            }
            prevPlus = plus; prevMinus = minus;
            if (ctrl && wheel != 0) {
                int target = dragId_;
                if (target < 0)
                    for (int i = static_cast<int>(boxes.size()) - 1; i >= 0 && target < 0; --i) {
                        const auto& b = boxes[static_cast<size_t>(i)];
                        if (cur.x >= b.a.x && cur.x <= b.b.x && cur.y >= b.a.y && cur.y <= b.b.y) target = b.id;
                    }
                float f = std::pow(1.1f, static_cast<float>(wheel) / 120.f);
                auto clampf = [](float v, float lo, float hi) { return std::min(std::max(v, lo), hi); };
                if (target == 0) s.hud.keys.scale = clampf(s.hud.keys.scale * f, 0.5f, 3.f);
                else if (target == 1) s.hud.cps.scale = clampf(s.hud.cps.scale * f, 0.5f, 4.f);
                else if (target == 2) s.hud.timer.scale = clampf(s.hud.timer.scale * f, 0.5f, 4.f);
                else if (target == 3) s.hud.watermark.scale = clampf(s.hud.watermark.scale * f, 0.05f, 5.f);
            }
        }
        guide_ = Vec2{-1.f, -1.f};
        if (dragId_ >= 0 && lb) {
            for (const auto& b : boxes) {
                if (b.id != dragId_) continue;
                float bw = b.b.x - b.a.x, bh = b.b.y - b.a.y;
                float nx = std::min(std::max(cur.x - grab_.x, aorg.x), aorg.x + aw - bw);
                float ny = std::min(std::max(cur.y - grab_.y, aorg.y), aorg.y + ah - bh);
                const float snap = 10.f;
                if (nx - aorg.x < snap) nx = aorg.x;
                else if (aorg.x + aw - (nx + bw) < snap) nx = aorg.x + aw - bw;
                else if (std::fabs(nx + bw * 0.5f - (aorg.x + aw * 0.5f)) < snap) { nx = aorg.x + aw * 0.5f - bw * 0.5f; guide_.x = aorg.x + aw * 0.5f; }
                if (ny - aorg.y < snap) ny = aorg.y;
                else if (aorg.y + ah - (ny + bh) < snap) ny = aorg.y + ah - bh;
                else if (std::fabs(ny + bh * 0.5f - (aorg.y + ah * 0.5f)) < snap) { ny = aorg.y + ah * 0.5f - bh * 0.5f; guide_.y = aorg.y + ah * 0.5f; }
                float px = (nx - aorg.x) / aw * 100.f, py = (ny - aorg.y) / ah * 100.f;
                if (dragId_ == 0) { s.hud.keys.posX = px; s.hud.keys.posY = py; }
                else if (dragId_ == 1) { s.hud.cps.posX = px; s.hud.cps.posY = py; }
                else if (dragId_ == 2) { s.hud.timer.posX = px; s.hud.timer.posY = py; }
                else { s.hud.watermark.posX = px; s.hud.watermark.posY = py; }
            }
        }
        if (dragId_ >= 0 && !lb) { dragId_ = -1; layoutChanged_ = true; }
    }

    // crosshair animation (spread on shoot / shrink on move)
    {
        const auto& ch0 = v.crosshair;
        bool moving = (GetAsyncKeyState('W') | GetAsyncKeyState('A') | GetAsyncKeyState('S') | GetAsyncKeyState('D')) & 0x8000;
        float target = 0.f;
        if (ch0.spreadOnShoot && lb) target += ch0.animAmount;
        if (ch0.shrinkOnMove && moving) target -= ch0.animAmount * 0.5f;
        if (ch0.easing == 1) xhExtra_ += (target - xhExtra_) * (1.f - std::exp(-ch0.animSpeed * dt));
        else {
            float step = ch0.animSpeed * std::max(ch0.animAmount, 1.f) * 0.5f * dt;
            xhExtra_ += std::min(std::max(target - xhExtra_, -step), step);
        }
        xhAnimating_ = std::fabs(target - xhExtra_) > 0.02f;
        if (!ch0.spreadOnShoot && !ch0.shrinkOnMove) { xhExtra_ = 0.f; xhAnimating_ = false; }
    }

    // ---- do we need to redraw? ----
    nlohmann::json keyJson = {v.fovCircle, v.crosshair, s.hud};   // (watermark path / position are part of s.hud)
    std::string key = keyJson.dump() + std::to_string(anchor.left) + "," + std::to_string(anchor.top) + "," +
                      std::to_string(anchor.right) + "," + std::to_string(anchor.bottom);
    bool staticChanged = key != staticKey_;
    bool active = engine_.Active() || hud_.Active(s.hud) || xhAnimating_ || editing_ || hasWm;
    topmostTimer_ += dt;
    if (topmostTimer_ > 1.f) { topmostTimer_ = 0.f; SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); }
    if (!staticChanged && !active && !wasActive_ && !needFull_) return;
    staticKey_ = key;
    wasActive_ = active;

    // ---- clear what we drew last time ----
    RECT full{0, 0, w_, h_};
    RECT clr = needFull_ ? full : prevDirty_;
    clr.left = std::max<LONG>(clr.left, 0); clr.top = std::max<LONG>(clr.top, 0);
    clr.right = std::min<LONG>(clr.right, w_); clr.bottom = std::min<LONG>(clr.bottom, h_);
    if (clr.right > clr.left) {
        for (LONG y = clr.top; y < clr.bottom; ++y)
            std::memset(static_cast<BYTE*>(bits_) + (static_cast<size_t>(y) * w_ + clr.left) * 4, 0, static_cast<size_t>(clr.right - clr.left) * 4);
    }

    // ---- draw ----
    RECT dirty{}; bool any = false;
    GdiSink sink(g_, &dirty, &any);
    float acx = (anchor.left + anchor.right) * 0.5f - rect_.left, acy = (anchor.top + anchor.bottom) * 0.5f - rect_.top;
    float halfW = (anchor.right - anchor.left) * 0.5f, halfH = (anchor.bottom - anchor.top) * 0.5f;

    const auto& f = v.fovCircle;
    if (f.enable) {
        Vec2 c{acx + f.centerX * 0.01f * halfW, acy + f.centerY * 0.01f * halfH};
        Rgba col{f.color[0], f.color[1], f.color[2], f.opacity};
        if (f.filled) sink.Circle(c, f.radius, Rgba{col.r, col.g, col.b, f.opacity * 0.2f}, true, 0.f);
        if (f.outline) sink.Circle(c, f.radius, col, false, std::max(1.f, f.thickness));
    }
    const auto& ch = v.crosshair;
    if (ch.enable) {
        Vec2 c{acx, acy};
        Rgba col{ch.color[0], ch.color[1], ch.color[2], ch.opacity};
        float th = std::max(1.f, ch.thickness);
        float gap = std::max(0.f, ch.gap + xhExtra_);
        if (ch.style == 0 || ch.style == 3) {
            sink.Line({c.x - gap - ch.size, c.y}, {c.x - gap, c.y}, th, col);
            sink.Line({c.x + gap, c.y}, {c.x + gap + ch.size, c.y}, th, col);
            sink.Line({c.x, c.y - gap - ch.size}, {c.x, c.y - gap}, th, col);
            sink.Line({c.x, c.y + gap}, {c.x, c.y + gap + ch.size}, th, col);
        }
        if (ch.style == 1 || ch.style == 3) sink.Circle(c, std::max(1.5f, th * 0.9f), col, true, 0.f);
        if (ch.style == 2) sink.Circle(c, gap + ch.size * 0.5f, col, false, th);
    }
    engine_.Render(sink, v.trail, 1.f);
    engine_.RenderParticles(sink, 1.f);
    hud_.Render(sink, s.hud, aorg, Vec2{aw, ah}, timerText);
    if (hasWm) {
        Gdiplus::ImageAttributes ia;
        Gdiplus::ColorMatrix cm = {1, 0, 0, 0, 0,  0, 1, 0, 0, 0,  0, 0, 1, 0, 0,  0, 0, 0, s.hud.watermark.opacity, 0,  0, 0, 0, 0, 1};
        ia.SetColorMatrix(&cm);
        g_->DrawImage(wmImg_, Gdiplus::RectF(static_cast<float>(wmRect.left), static_cast<float>(wmRect.top),
                      static_cast<float>(wmRect.right - wmRect.left), static_cast<float>(wmRect.bottom - wmRect.top)),
                      0.f, 0.f, static_cast<float>(wmImg_->GetWidth()), static_cast<float>(wmImg_->GetHeight()), Gdiplus::UnitPixel, &ia);
        Touch(dirty, any, static_cast<float>(wmRect.left), static_cast<float>(wmRect.top), static_cast<float>(wmRect.right), static_cast<float>(wmRect.bottom));
    }
    if (editing_) {
        // faint tint so the overlay catches the mouse, element frames, snap guides and the Done button
        Gdiplus::SolidBrush tint(Gdiplus::Color(34, 0, 0, 0));
        g_->FillRectangle(&tint, 0, 0, w_, h_);
        Touch(dirty, any, 0.f, 0.f, static_cast<float>(w_), static_cast<float>(h_));
        static const char* names[4] = {"Key Overlay", "CPS", "Timer", "Watermark"};
        for (const auto& b : boxes) {
            bool drag = (b.id == dragId_);
            sink.RoundRect(Vec2{b.a.x - 4.f, b.a.y - 4.f}, Vec2{b.b.x + 4.f, b.b.y + 4.f}, 8.f, Rgba{0.f, 0.9f, 1.f, drag ? 0.22f : 0.10f}, Rgba{0.f, 0.9f, 1.f, 0.95f}, 2.f);
            sink.Text(Vec2{b.a.x, b.a.y - 16.f}, names[b.id], 13.f, Rgba{0.7f, 1.f, 1.f, 1.f}, 0, true);
        }
        if (guide_.x >= 0.f) sink.Line(Vec2{guide_.x, aorg.y}, Vec2{guide_.x, aorg.y + ah}, 1.f, Rgba{0.f, 0.9f, 1.f, 0.7f});
        if (guide_.y >= 0.f) sink.Line(Vec2{aorg.x, guide_.y}, Vec2{aorg.x + aw, guide_.y}, 1.f, Rgba{0.f, 0.9f, 1.f, 0.7f});
        float dbx = aorg.x + aw * 0.5f - 60.f, dby = aorg.y + 14.f;
        sink.RoundRect(Vec2{dbx, dby}, Vec2{dbx + 120.f, dby + 34.f}, 10.f, Rgba{0.f, 0.9f, 1.f, 0.95f}, Rgba{1.f, 1.f, 1.f, 0.5f}, 1.f);
        sink.Text(Vec2{dbx + 60.f, dby + 17.f}, "Done", 15.f, Rgba{0.02f, 0.05f, 0.08f, 1.f}, 1, true);
        sink.Text(Vec2{aorg.x + aw * 0.5f, dby + 52.f}, "Layout edit mode - drag the HUD elements (they snap to edges and centre). Ctrl + mouse wheel resizes. Esc or Done to finish.", 14.f, Rgba{1.f, 1.f, 1.f, 0.95f}, 1, true);
    }

    // ---- push the dirty area to the screen ----
    RECT u = needFull_ ? full : prevDirty_;
    if (any) {
        if (needFull_ || (u.right <= u.left)) u = dirty;
        else if (!needFull_) {
            u.left = std::min(u.left, dirty.left); u.top = std::min(u.top, dirty.top);
            u.right = std::max(u.right, dirty.right); u.bottom = std::max(u.bottom, dirty.bottom);
        }
    }
    if (needFull_) u = full;
    u.left = std::max<LONG>(u.left, 0); u.top = std::max<LONG>(u.top, 0);
    u.right = std::min<LONG>(u.right, w_); u.bottom = std::min<LONG>(u.bottom, h_);
    prevDirty_ = any ? dirty : RECT{};
    if (u.right > u.left && u.bottom > u.top) {
        POINT dst{rect_.left, rect_.top}, src{0, 0};
        SIZE sz{w_, h_};
        BLENDFUNCTION bf{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        UPDATELAYEREDWINDOWINFO ul{};
        ul.cbSize = sizeof(ul);
        ul.pptDst = &dst; ul.psize = &sz; ul.hdcSrc = memDC_; ul.pptSrc = &src;
        ul.pblend = &bf; ul.dwFlags = ULW_ALPHA; ul.prcDirty = &u;
        if (!UpdateLayeredWindowIndirect(hwnd_, &ul))
            UpdateLayeredWindow(hwnd_, nullptr, &dst, &sz, memDC_, &src, 0, &bf, ULW_ALPHA);
    }
    if (needFull_ && hwnd_) ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    // Developer diagnostic: HATENIUM_OVERLAY_DUMP=<file> writes the raw overlay bitmap (w, h, BGRA premultiplied) about once a second.
    {
        static wchar_t dumpPath[MAX_PATH] = {};
        static bool dumpInit = false;
        if (!dumpInit) { dumpInit = true; GetEnvironmentVariableW(L"HATENIUM_OVERLAY_DUMP", dumpPath, MAX_PATH); }
        dumpTimer_ += dt;
        if (dumpPath[0] && dumpTimer_ > 1.f) {
            dumpTimer_ = 0.f;
            FILE* f = _wfopen(dumpPath, L"wb");
            if (f) { int hdr[2] = {w_, h_}; fwrite(hdr, sizeof(hdr), 1, f); fwrite(bits_, 4, static_cast<size_t>(w_) * h_, f); fclose(f); }
        }
    }
    needFull_ = false;
}

} // namespace hat
