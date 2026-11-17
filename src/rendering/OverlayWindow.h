#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "core/Settings.h"
#include "rendering/Effects.h"
#include "rendering/Hud.h"

namespace Gdiplus { class Bitmap; class Graphics; class Image; }

namespace hat {

// A transparent, click-through, always-on-top window that Hatenium draws into with GDI+.
// It only paints pixels (FOV circle, crosshair, mouse trail, click particles). It never touches other processes:
// the cursor position and mouse-button state come from GetCursorPos / GetAsyncKeyState.
class OverlayWindow {
public:
    bool Init(HINSTANCE inst);
    void Shutdown();
    // editMode: HUD layout editing (the overlay becomes interactive); the overlay clears it when the user finishes.
    void Update(float dt, Settings& s, bool& editMode, const std::string& timerText);
    const std::string& MonitorKey() const { return monKey_; }
    bool ConsumeMonitorKeyChanged() { bool v = monKeyChanged_; monKeyChanged_ = false; return v; }
    bool ConsumeLayoutChanged() { bool v = layoutChanged_; layoutChanged_ = false; return v; }
    bool Visible() const { return hwnd_ != nullptr; }

    static int MonitorCount();
    static int PrimaryScreenHeight();

private:
    bool EnsureWindow(const RECT& r);
    void DestroyWindowAndBuffers();
    void CreateBuffers(int w, int h);

    HWND hwnd_ = nullptr;
    HINSTANCE inst_ = nullptr;
    ULONG_PTR gdiToken_ = 0;
    bool gdiOk_ = false;
    HDC memDC_ = nullptr;
    HBITMAP dib_ = nullptr, oldBmp_ = nullptr;
    void* bits_ = nullptr;
    int w_ = 0, h_ = 0;
    RECT rect_{};
    Gdiplus::Bitmap* bmp_ = nullptr;
    Gdiplus::Graphics* g_ = nullptr;

    EffectsEngine engine_;
    HudRenderer hud_;
    float xhExtra_ = 0.f;
    bool xhAnimating_ = false;
    RECT prevDirty_{};
    bool needFull_ = true, wasActive_ = false;
    std::string staticKey_;
    float topmostTimer_ = 0.f, dumpTimer_ = 0.f;

    // layout editor + watermark
    bool editing_ = false, prevLbEdit_ = false, monKeyChanged_ = false, layoutChanged_ = false;
    int dragId_ = -1;
    Vec2 grab_{};
    Vec2 guide_{-1.f, -1.f};
    std::string monKey_;
    Gdiplus::Image* wmImg_ = nullptr;
    std::wstring wmPath_;
    void SetInteractive(bool on);
};

} // namespace hat
