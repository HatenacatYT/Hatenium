#include "core/WindowManager.h"
#include "imgui.h"
#include <windowsx.h>
#include <commdlg.h>
#include <algorithm>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace hat {

namespace {
const wchar_t* kClassName = L"HateniumWindowClass";

template <typename Fn> Fn LoadUser32(const char* name) {
    HMODULE m = GetModuleHandleW(L"user32.dll");
    return m ? reinterpret_cast<Fn>(reinterpret_cast<void*>(GetProcAddress(m, name))) : nullptr;
}
} // namespace

// ---------------------------------------------------------------- statics
void WindowManager::EnableDpiAwareness() {
    using Fn = BOOL(WINAPI*)(HANDLE);
    if (auto fn = LoadUser32<Fn>("SetProcessDpiAwarenessContext")) {
        if (fn(reinterpret_cast<HANDLE>(-4))) return;          // PER_MONITOR_AWARE_V2
    }
    SetProcessDPIAware();
}

float WindowManager::SystemDpiScale() {
    using Fn = UINT(WINAPI*)();
    if (auto fn = LoadUser32<Fn>("GetDpiForSystem")) return fn() / 96.f;
    HDC dc = GetDC(nullptr);
    int dpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(nullptr, dc);
    return dpi / 96.f;
}

RECT WindowManager::PrimaryWorkArea() {
    RECT r{0, 0, 1280, 720};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &r, 0);
    return r;
}

RECT WindowManager::WorkAreaFor(const RECT& rc) {
    HMONITOR m = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (m && GetMonitorInfoW(m, &mi)) return mi.rcWork;
    return PrimaryWorkArea();
}

RECT WindowManager::ClampToWork(RECT r, const RECT& work) {
    int w = std::min<int>(r.right - r.left, work.right - work.left);
    int h = std::min<int>(r.bottom - r.top, work.bottom - work.top);
    int x = std::min<int>(std::max<int>(r.left, work.left), work.right - w);
    int y = std::min<int>(std::max<int>(r.top, work.top), work.bottom - h);
    return RECT{x, y, x + w, y + h};
}

// ---------------------------------------------------------------- lifetime
bool WindowManager::Create(HINSTANCE inst, const RECT& r, const wchar_t* title) {
    inst_ = inst;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProcStatic;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kClassName, title,
                            WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_SYSMENU,
                            r.left, r.top, r.right - r.left, r.bottom - r.top,
                            nullptr, nullptr, inst, this);
    if (!hwnd_) return false;

    using Fn = UINT(WINAPI*)(HWND);
    if (auto fn = LoadUser32<Fn>("GetDpiForWindow")) dpi_ = fn(hwnd_);
    else dpi_ = static_cast<UINT>(SystemDpiScale() * 96.f);
    return true;
}

void WindowManager::Destroy() {
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    UnregisterClassW(kClassName, inst_);
}

void WindowManager::Show() {
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);
}

void WindowManager::PumpMessages() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) quit_ = true;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void WindowManager::Minimize() { ShowWindow(hwnd_, SW_MINIMIZE); }

// ---------------------------------------------------------------- geometry
void WindowManager::SetRect(const RECT& r) {
    SetWindowPos(hwnd_, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

RECT WindowManager::GetRect() const {
    RECT r{};
    GetWindowRect(hwnd_, &r);
    return r;
}

void WindowManager::SetRoundRadius(int px) {
    RECT r = GetRect();
    int w = r.right - r.left, h = r.bottom - r.top;
    if (w == lastRgnW_ && h == lastRgnH_ && px == lastRgnR_) return;
    lastRgnW_ = w; lastRgnH_ = h; lastRgnR_ = px;
    if (px <= 0) { SetWindowRgn(hwnd_, nullptr, TRUE); return; }
    HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, px * 2, px * 2);
    SetWindowRgn(hwnd_, rgn, TRUE);   // the system owns the region afterwards
}

void WindowManager::BeginDragMove() {
    ReleaseCapture();
    SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

bool WindowManager::ConsumeResize(UINT& w, UINT& h) {
    if (!resizePending_) return false;
    resizePending_ = false; w = pendingW_; h = pendingH_;
    return true;
}
bool WindowManager::ConsumeDpiChanged() { bool v = dpiChanged_; dpiChanged_ = false; return v; }
bool WindowManager::ConsumeDisplayChanged() { bool v = displayChanged_; displayChanged_ = false; return v; }
bool WindowManager::ConsumeMoveSizeFinished() { bool v = moveSizeDone_; moveSizeDone_ = false; return v; }

// ---------------------------------------------------------------- dialogs
std::wstring WindowManager::FileDialog(bool save, const wchar_t* defaultName) {
    wchar_t file[MAX_PATH] = {};
    if (defaultName) wcsncpy(file, defaultName, MAX_PATH - 1);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"Hatenium config (*.json)\0*.json\0All files (*.*)\0*.*\0\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | (save ? OFN_OVERWRITEPROMPT : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST));
    BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    return ok ? std::wstring(file) : std::wstring();
}

// ---------------------------------------------------------------- window proc
LRESULT CALLBACK WindowManager::WndProcStatic(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    WindowManager* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<WindowManager*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<WindowManager*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self) return self->WndProc(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return 1;

    switch (msg) {
    case WM_NCCALCSIZE:
        if (wp) return 0;                 // no non-client area: the client covers the whole window
        break;
    case WM_NCHITTEST: {
        if (resizable_ && !IsZoomed(hwnd)) {
            POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int b = std::max(5, static_cast<int>(6.f * dpi_ / 96.f));
            bool L = pt.x < rc.left + b, R = pt.x >= rc.right - b;
            bool T = pt.y < rc.top + b,  B = pt.y >= rc.bottom - b;
            if (T && L) return HTTOPLEFT;
            if (T && R) return HTTOPRIGHT;
            if (B && L) return HTBOTTOMLEFT;
            if (B && R) return HTBOTTOMRIGHT;
            if (L) return HTLEFT;
            if (R) return HTRIGHT;
            if (T) return HTTOP;
            if (B) return HTBOTTOM;
        }
        return HTCLIENT;
    }
    case WM_GETMINMAXINFO:
        if (minW_ > 0 && minH_ > 0) {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
            mmi->ptMinTrackSize.x = minW_;
            mmi->ptMinTrackSize.y = minH_;
            return 0;
        }
        break;
    case WM_SIZE:
        if (wp == SIZE_MINIMIZED) {
            minimized_ = true;
        } else {
            minimized_ = false;
            pendingW_ = LOWORD(lp); pendingH_ = HIWORD(lp);
            resizePending_ = true;
        }
        return 0;
    case WM_ENTERSIZEMOVE: inSizeMove_ = true; return 0;
    case WM_EXITSIZEMOVE:  inSizeMove_ = false; moveSizeDone_ = true; return 0;
    case WM_DPICHANGED: {
        dpi_ = HIWORD(wp);
        const RECT* r = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        dpiChanged_ = true;
        return 0;
    }
    case WM_DISPLAYCHANGE: displayChanged_ = true; return 0;
    case WM_SETTINGCHANGE:
        if (wp == SPI_SETWORKAREA) displayChanged_ = true;
        break;
    case WM_ERASEBKGND: return 1;
    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0;   // no ALT menu beep
        break;
    case WM_CLOSE: closeRequested_ = true; return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace hat
