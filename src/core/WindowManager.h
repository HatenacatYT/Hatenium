#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace hat {

// Borderless, resizable, rounded Win32 window with DPI awareness and screen-boundary helpers.
class WindowManager {
public:
    static void  EnableDpiAwareness();
    static float SystemDpiScale();
    static RECT  PrimaryWorkArea();
    static RECT  WorkAreaFor(const RECT& r);
    static RECT  ClampToWork(RECT r, const RECT& work);   // shrinks (if needed) and moves fully on-screen

    bool Create(HINSTANCE inst, const RECT& r, const wchar_t* title);
    void Destroy();
    HWND Hwnd() const { return hwnd_; }
    void Show();
    void PumpMessages();
    bool QuitRequested() const { return quit_; }
    bool CloseRequested() const { return closeRequested_; }
    void RequestClose() { closeRequested_ = true; }
    void Minimize();
    void SetTopmost(bool on);
    bool Topmost() const { return topmost_; }
    bool IsMinimized() const { return minimized_; }

    void SetRect(const RECT& r);
    RECT GetRect() const;
    void SetRoundRadius(int px);
    void SetResizable(bool v) { resizable_ = v; }
    void SetMinSize(int w, int h) { minW_ = w; minH_ = h; }
    void BeginDragMove();
    float DpiScale() const { return dpi_ / 96.f; }
    bool InSizeMove() const { return inSizeMove_; }

    bool ConsumeResize(UINT& w, UINT& h);
    bool ConsumeDpiChanged();
    bool ConsumeDisplayChanged();
    bool ConsumeMoveSizeFinished();

    std::wstring FileDialog(bool save, const wchar_t* defaultName);
    std::vector<std::wstring> OpenMusicFilesDialog();
    std::wstring FolderDialog();
    std::wstring ImageDialog();
    std::vector<std::wstring> TakeDroppedFiles() { std::vector<std::wstring> v; v.swap(dropped_); return v; }

private:
    static LRESULT CALLBACK WndProcStatic(HWND, UINT, WPARAM, LPARAM);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);

    HWND hwnd_ = nullptr;
    HINSTANCE inst_ = nullptr;
    UINT dpi_ = 96;
    bool quit_ = false, closeRequested_ = false, minimized_ = false, resizable_ = false, inSizeMove_ = false;
    bool resizePending_ = false, dpiChanged_ = false, displayChanged_ = false, moveSizeDone_ = false;
    UINT pendingW_ = 0, pendingH_ = 0;
    std::vector<std::wstring> dropped_;
    bool mainDestroyed_ = false;
    bool topmost_ = false;
    int minW_ = 0, minH_ = 0;
    int lastRgnW_ = -1, lastRgnH_ = -1, lastRgnR_ = -1;
};

} // namespace hat
