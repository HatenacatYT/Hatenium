#include "core/Application.h"
#include "core/AnimationManager.h"
#include "core/Localization.h"
#include "gui/Components.h"
#include "gui/Screens.h"
#include "gui/ThemeManager.h"
#include "gui/LogoData.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>

namespace hat {

namespace {
constexpr float kMinSplashSeconds = 2.0f;
constexpr int kMinMainW = 720, kMinMainH = 500;       // logical pixels
constexpr float kDefaultMainW = 1000.f, kDefaultMainH = 640.f;
constexpr int kInitSteps = 4;

bool FileExists(const std::string& p) {
    DWORD a = GetFileAttributesA(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}
std::string ExeDirA() {
    char buf[MAX_PATH * 2];
    DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    std::string p(buf, n);
    size_t s = p.find_last_of("\\/");
    return s == std::string::npos ? std::string(".") : p.substr(0, s);
}
} // namespace

// ============================================================================ lifetime
int Application::Run(HINSTANCE inst) {
    WindowManager::EnableDpiAwareness();
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);   // folder dialog / shell
    config_.Init();
    config_.Load(settings_);
    startupMessage_ = config_.TakeMessage();
    AnimationManager::Get().Configure(settings_.misc.uiAnimation, settings_.animationSpeed);
    SetLanguage(settings_.language);

    themes_.Load(config_.Dir());
    layout_.Load(config_.Dir());
    playtime_.Load(config_.Dir());
    {
        wchar_t exe[MAX_PATH * 2];
        DWORD n = GetModuleFileNameW(nullptr, exe, MAX_PATH * 2);
        region_.SetDataDir(config_.Dir(), std::filesystem::path(std::wstring(exe, n)).parent_path());
    }

    if (!InitGraphics(inst)) return 1;
    lastSaved_ = ConfigManager::Serialize(settings_);

    auto prev = std::chrono::steady_clock::now();
    const float clear[4] = {0.031f, 0.043f, 0.071f, 1.f};

    while (!wm_.QuitRequested()) {
        wm_.PumpMessages();
        if (wm_.QuitRequested()) break;
        if (wm_.CloseRequested()) {
            StoreWindowState();
            profiles_.Release(settings_);
            SaveSettings(false);
            playtime_.Flush();
            wm_.Destroy();             // posts WM_QUIT; the loop ends on the next check
            wm_.PumpMessages();
            break;
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::min(std::chrono::duration<float>(now - prev).count(), 0.1f);
        prev = now;

        if (wm_.IsMinimized()) { BackgroundTick(dt); Sleep(8); continue; }

        AnimationManager& am = AnimationManager::Get();
        am.Configure(settings_.misc.uiAnimation, settings_.animationSpeed);
        am.Update(dt);
        BackgroundTick(dt);
        SyncScaleAndLanguage();
        UpdateSplashTasks(dt);
        UpdateTransition();
        HandleWindowEvents();
        EnforceMinSize();

        UINT w = 0, h = 0;
        if (wm_.ConsumeResize(w, h)) renderer_.Resize(w, h);
        if (fontsDirty_) RebuildFonts();

        ThemeManager::Get().Update(settings_, dt);
        ui::SetNotificationsEnabled(settings_.misc.notifications);
        wm_.SetRoundRadius(static_cast<int>(std::lround(ThemeManager::Get().T().windowRounding)));

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        DrawUI();
        ImGui::Render();

        renderer_.BeginFrame(clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        fps_ = ImGui::GetIO().Framerate;
        if (!renderer_.Present(true)) Sleep(20);   // occluded

        AutoSave(dt);
    }

    Shutdown();
    return 0;
}

bool Application::InitGraphics(HINSTANCE inst) {
    RECT work = WindowManager::PrimaryWorkArea();
    if (!wm_.Create(inst, RectFor(Screen::Splash, work), L"Hatenium")) {
        MessageBoxW(nullptr, L"Could not create the window.", L"Hatenium", MB_OK | MB_ICONERROR);
        return false;
    }
    wm_.SetRect(RectFor(Screen::Splash, work));      // again, now with the window's real DPI
    if (!renderer_.Init(wm_.Hwnd())) {
        MessageBoxW(nullptr, L"Direct3D 11 could not be initialised.", L"Hatenium", MB_OK | MB_ICONERROR);
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    if (!ImGui_ImplWin32_Init(wm_.Hwnd()) || !ImGui_ImplDX11_Init(renderer_.Device(), renderer_.Context())) {
        MessageBoxW(nullptr, L"Dear ImGui could not be initialised.", L"Hatenium", MB_OK | MB_ICONERROR);
        return false;
    }
    imguiReady_ = true;

    scale_ = wm_.DpiScale() * settings_.uiScale;
    ThemeManager::Get().SetScale(scale_);
    RebuildFonts();

    logoSrv_ = renderer_.CreateTextureRGBA(kLogoRgba, kLogoW, kLogoH);
    if (logoSrv_) ThemeManager::Get().SetLogoTexture((ImTextureID)(uintptr_t)logoSrv_);
    overlay_.Init(inst);
    if (!music_.Init()) { /* no audio device: the Music tab explains it */ }
    ThemeManager::Get().Update(settings_, 0.f, true);
    wm_.SetRoundRadius(static_cast<int>(std::lround(ThemeManager::Get().T().windowRounding)));
    wm_.Show();
    return true;
}

void Application::Shutdown() {
    SetThreadExecutionState(ES_CONTINUOUS);
    youtube_.Close();
    overlay_.Shutdown();
    music_.Shutdown();
    if (logoSrv_) { logoSrv_->Release(); logoSrv_ = nullptr; }
    if (imguiReady_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiReady_ = false;
    }
    renderer_.Shutdown();
    wm_.Destroy();
}

// ============================================================================ background work
void Application::BackgroundTick(float dt) {
    if (wm_.Hwnd() && settings_.alwaysOnTop != wm_.Topmost()) wm_.SetTopmost(settings_.alwaysOnTop);
    tools_.Update(dt);
    if (tools_.ConsumeTimerFinished()) { ui::Notify(tr("Timer finished!"), 1); MessageBeep(MB_ICONASTERISK); }
    {
        std::string pname;
        if (profiles_.Update(dt, settings_, pname))
            ui::Notify(pname.empty() ? std::string(tr("Profile off - back to your setup"))
                                     : std::string(tr("Profile")) + ": " + pname, 0);
    }
    playtime_.Update(dt, profiles_.Processes(), settings_.tools);
    overlay_.Update(dt, settings_, hudEdit_, tools_.DisplayText());
    if (overlay_.ConsumeMonitorKeyChanged()) layout_.Apply(overlay_.MonitorKey(), settings_.hud);
    if (overlay_.ConsumeLayoutChanged()) {
        layout_.Store(overlay_.MonitorKey(), settings_.hud);
        layout_.Save();
        ui::Notify(tr("Layout saved"), 1);
    }
    music_.Update(settings_.music);
    youtube_.Update();

    bool want = settings_.misc.preventSleep;
    if (want != sleepBlocked_) {
        SetThreadExecutionState(want ? (ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED) : ES_CONTINUOUS);
        sleepBlocked_ = want;
    }
    auto dropped = wm_.TakeDroppedFiles();
    if (!dropped.empty()) {
        size_t before = settings_.music.tracks.size();
        music_.AddPaths(settings_.music, dropped);
        size_t added = settings_.music.tracks.size() - before;
        ui::Notify(std::to_string(added) + " " + tr("tracks added"), added ? 1 : 2);
    }
}

void Application::StartLayoutEdit() {
    if (!settings_.visuals.overlay.enable) settings_.visuals.overlay.enable = true;
    hudEdit_ = true;
    ui::Notify(tr("Layout edit mode: drag the HUD on your screen"), 0);
}

void Application::ExportPreset() {
    std::wstring path = wm_.FileDialog(true, L"hatenium-preset.json");
    if (path.empty()) return;
    std::string err = config_.ExportPreset(settings_, std::filesystem::path(path));
    ui::Notify(err.empty() ? std::string(tr("Preset exported")) : std::string(tr(err.c_str())), err.empty() ? 1 : 2);
}

void Application::ImportPreset() {
    std::wstring path = wm_.FileDialog(false, nullptr);
    if (path.empty()) return;
    std::string err = config_.ImportPreset(settings_, std::filesystem::path(path));
    ui::Notify(err.empty() ? std::string(tr("Preset imported")) : std::string(tr(err.c_str())), err.empty() ? 1 : 2);
}

void Application::ExportTheme(int index) {
    std::wstring path = wm_.FileDialog(true, L"hatenium-theme.json");
    if (path.empty()) return;
    std::string err = themes_.Export(index, std::filesystem::path(path));
    ui::Notify(err.empty() ? std::string(tr("Theme exported")) : std::string(tr(err.c_str())), err.empty() ? 1 : 2);
}

void Application::ImportTheme() {
    std::wstring path = wm_.FileDialog(false, nullptr);
    if (path.empty()) return;
    std::string err = themes_.Import(std::filesystem::path(path));
    if (err.empty()) themes_.Save();
    ui::Notify(err.empty() ? std::string(tr("Theme imported")) : std::string(tr(err.c_str())), err.empty() ? 1 : 2);
}

void Application::AddMusicFiles() {
    auto files = wm_.OpenMusicFilesDialog();
    if (files.empty()) return;
    size_t before = settings_.music.tracks.size();
    music_.AddPaths(settings_.music, files);
    ui::Notify(std::to_string(settings_.music.tracks.size() - before) + " " + tr("tracks added"), 1);
}

void Application::AddMusicFolder() {
    std::wstring dir = wm_.FolderDialog();
    if (dir.empty()) return;
    size_t before = settings_.music.tracks.size();
    music_.AddPaths(settings_.music, {dir});
    ui::Notify(std::to_string(settings_.music.tracks.size() - before) + " " + tr("tracks added"), 1);
}

// ============================================================================ fonts / scale
void Application::RebuildFonts() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDX11_InvalidateDeviceObjects();
    io.Fonts->Clear();

    char winDir[MAX_PATH] = {};
    GetWindowsDirectoryA(winDir, MAX_PATH);
    const std::string sys = std::string(winDir) + "\\Fonts\\";
    const std::string bundled = ExeDirA() + "\\assets\\fonts\\";
    const float px = std::round(15.f * scale_), logoPx = std::round(34.f * scale_);

    ImFontConfig cfg;
    cfg.OversampleH = 2; cfg.OversampleV = 1; cfg.PixelSnapH = true;
    ImFont* mainFont = nullptr;
    for (const std::string& f : {bundled + "Hatenium-Regular.ttf", sys + "segoeui.ttf"}) {
        if (!FileExists(f)) continue;
        mainFont = io.Fonts->AddFontFromFileTTF(f.c_str(), px, &cfg, io.Fonts->GetGlyphRangesDefault());
        if (mainFont) break;
    }
    if (!mainFont) {
        ImFontConfig dc; dc.SizePixels = px;
        mainFont = io.Fonts->AddFontDefault(&dc);
    }
    if (settings_.language == 1) {   // Japanese glyphs only when needed (keeps the atlas small)
        ImFontConfig m;
        m.MergeMode = true; m.OversampleH = 1; m.OversampleV = 1;
        for (const char* f : {"meiryo.ttc", "YuGothM.ttc", "msgothic.ttc"}) {
            std::string path = sys + f;
            if (FileExists(path) && io.Fonts->AddFontFromFileTTF(path.c_str(), px, &m, io.Fonts->GetGlyphRangesJapanese())) break;
        }
    }
    static const ImWchar logoRanges[] = {0x20, 0x7E, 0};
    ImFont* logoFont = nullptr;
    ImFontConfig lc; lc.OversampleH = 2; lc.OversampleV = 2;
    for (const std::string& f : {bundled + "Hatenium-Bold.ttf", sys + "segoeuib.ttf"}) {
        if (!FileExists(f)) continue;
        logoFont = io.Fonts->AddFontFromFileTTF(f.c_str(), logoPx, &lc, logoRanges);
        if (logoFont) break;
    }
    if (!logoFont) logoFont = mainFont;

    io.FontDefault = mainFont;
    ThemeManager::Get().SetLogoFont(logoFont);
    fontsLanguage_ = settings_.language;
    fontsDirty_ = false;
}

void Application::SyncScaleAndLanguage() {
    float want = wm_.DpiScale() * settings_.uiScale;
    if (std::fabs(want - scale_) > 1e-3f) {
        scale_ = want;
        ThemeManager::Get().SetScale(scale_);
        fontsDirty_ = true;
    }
    if (Language() != settings_.language) SetLanguage(settings_.language);
    if (fontsLanguage_ != settings_.language) fontsDirty_ = true;
}

// ============================================================================ geometry
int Application::MinMainW() const { return static_cast<int>(kMinMainW * scale_); }
int Application::MinMainH() const { return static_cast<int>(kMinMainH * scale_); }

RECT Application::RectFor(Screen s, const RECT& around) const {
    float dpi = wm_.Hwnd() ? wm_.DpiScale() : WindowManager::SystemDpiScale();
    RECT work = WindowManager::WorkAreaFor(around);
    int ww = work.right - work.left, wh = work.bottom - work.top;
    int cx = (around.left + around.right) / 2, cy = (around.top + around.bottom) / 2;
    int w = 0, h = 0;
    bool useSavedPos = false;

    switch (s) {
    case Screen::Splash: {
        w = static_cast<int>(640 * dpi); h = static_cast<int>(360 * dpi);
        if (w > ww * 0.8f) { w = static_cast<int>(ww * 0.8f); h = w * 9 / 16; }
        if (h > wh * 0.8f) { h = static_cast<int>(wh * 0.8f); w = h * 16 / 9; }
        cx = (work.left + work.right) / 2; cy = (work.top + work.bottom) / 2;
        break; }
    case Screen::Setup: {
        h = static_cast<int>(std::min(780.f * dpi, wh * 0.92f)); w = h * 9 / 16;
        break; }
    case Screen::Main: {
        float lw = kDefaultMainW, lh = kDefaultMainH;
        if (settings_.rememberSize && settings_.windowW > 0 && settings_.windowH > 0) { lw = float(settings_.windowW); lh = float(settings_.windowH); }
        w = static_cast<int>(lw * dpi); h = static_cast<int>(lh * dpi);
        w = std::max(w, std::min(MinMainW(), ww)); h = std::max(h, std::min(MinMainH(), wh));
        w = std::min(w, static_cast<int>(ww * 0.98f)); h = std::min(h, static_cast<int>(wh * 0.96f));
        useSavedPos = settings_.rememberPosition && settings_.hasWindowPos;
        break; }
    }
    RECT r;
    if (useSavedPos) r = RECT{settings_.windowX, settings_.windowY, settings_.windowX + w, settings_.windowY + h};
    else             r = RECT{cx - w / 2, cy - h / 2, cx - w / 2 + w, cy - h / 2 + h};
    return WindowManager::ClampToWork(r, WindowManager::WorkAreaFor(r));
}

void Application::StoreWindowState() {
    if (screen_ != Screen::Main || transitioning_ || wm_.InSizeMove()) return;
    RECT r = wm_.GetRect();
    float dpi = wm_.DpiScale();
    settings_.windowX = r.left; settings_.windowY = r.top; settings_.hasWindowPos = true;
    settings_.windowW = static_cast<int>(std::lround((r.right - r.left) / dpi));
    settings_.windowH = static_cast<int>(std::lround((r.bottom - r.top) / dpi));
}

void Application::KeepOnScreen() {
    RECT r = wm_.GetRect();
    RECT c = WindowManager::ClampToWork(r, WindowManager::WorkAreaFor(r));
    if (!EqualRect(&r, &c)) wm_.SetRect(c);
}

void Application::EnforceMinSize() {
    if (screen_ != Screen::Main || transitioning_) return;
    RECT r = wm_.GetRect();
    RECT work = WindowManager::WorkAreaFor(r);
    int minW = std::min(MinMainW(), static_cast<int>(work.right - work.left));
    int minH = std::min(MinMainH(), static_cast<int>(work.bottom - work.top));
    wm_.SetMinSize(minW, minH);
    if (wm_.InSizeMove()) return;
    int w = r.right - r.left, h = r.bottom - r.top;
    if (w < minW || h < minH) {
        RECT n{r.left, r.top, r.left + std::max(w, minW), r.top + std::max(h, minH)};
        wm_.SetRect(WindowManager::ClampToWork(n, work));
    }
}

void Application::HandleWindowEvents() {
    if (wm_.ConsumeDpiChanged() || wm_.ConsumeDisplayChanged()) {
        if (transitioning_) {                        // finish the running transition on the new monitor/DPI
            rectTo_ = RectFor(next_, wm_.GetRect());
            wm_.SetRect(rectTo_);
            AnimationManager::Get().Channel("stage").Snap(1.f);
            FinishTransition();
        } else if (screen_ == Screen::Main) {
            KeepOnScreen();
        } else {
            wm_.SetRect(RectFor(screen_, wm_.GetRect()));
        }
    }
    if (wm_.ConsumeMoveSizeFinished()) {
        KeepOnScreen();
        StoreWindowState();
    }
}

void Application::MaxLogicalWindowSize(int& w, int& h) const {
    RECT work = WindowManager::WorkAreaFor(wm_.GetRect());
    float dpi = wm_.DpiScale();
    w = static_cast<int>((work.right - work.left) * 0.98f / dpi);
    h = static_cast<int>((work.bottom - work.top) * 0.98f / dpi);
}

void Application::ResizeMainWindow(int lw, int lh) {
    if (screen_ != Screen::Main || transitioning_) return;
    float dpi = wm_.DpiScale();
    RECT r = wm_.GetRect();
    RECT work = WindowManager::WorkAreaFor(r);
    int w = std::max(static_cast<int>(lw * dpi), std::min(MinMainW(), static_cast<int>(work.right - work.left)));
    int h = std::max(static_cast<int>(lh * dpi), std::min(MinMainH(), static_cast<int>(work.bottom - work.top)));
    RECT n{r.left, r.top, r.left + w, r.top + h};
    wm_.SetRect(WindowManager::ClampToWork(n, work));
    StoreWindowState();
}

// ============================================================================ stage machine
void Application::UpdateSplashTasks(float dt) {
    if (screen_ != Screen::Splash) return;
    splashTime_ += dt;
    static const char* names[kInitSteps] = {"Loading configuration", "Applying theme", "Preparing interface", "Ready"};
    if (initStep_ < kInitSteps) {
        switch (initStep_) {
        case 0: Sanitize(settings_); break;
        case 1: ThemeManager::Get().Update(settings_, 0.f, true); break;
        case 2: break;   // window rects are computed on demand
        default: splashDone_ = true; break;
        }
        splashStatus_ = names[initStep_];
        ++initStep_;
    }
    float target = std::min(static_cast<float>(initStep_) / kInitSteps, splashTime_ / kMinSplashSeconds);
    splashBar_ += (target - splashBar_) * (1.f - std::exp(-8.f * dt));
    if (splashDone_ && splashTime_ >= kMinSplashSeconds && !transitioning_) {
        splashBar_ = 1.f;
        BeginTransition(settings_.firstLaunch ? Screen::Setup : Screen::Main);
    }
}

void Application::BeginTransition(Screen next) {
    if (transitioning_) return;              // ignore requests while another transition is running
    next_ = next;
    rectFrom_ = wm_.GetRect();
    rectTo_ = RectFor(next, rectFrom_);
    float dur = settings_.startupAnimation ? AnimationManager::Get().Duration(0.8f) : 0.f;
    AnimationManager::Get().Channel("stage").Start(0.f, 1.f, dur, Ease::Linear);
    transitioning_ = true;
    wm_.SetResizable(false);
    wm_.SetMinSize(0, 0);
}

void Application::UpdateTransition() {
    if (!transitioning_) return;
    Tween& tw = AnimationManager::Get().Channel("stage");
    float t = ApplyEase(Ease::InOutCubic, tw.Progress());
    RECT r{
        static_cast<LONG>(Lerp(float(rectFrom_.left), float(rectTo_.left), t)),
        static_cast<LONG>(Lerp(float(rectFrom_.top), float(rectTo_.top), t)),
        static_cast<LONG>(Lerp(float(rectFrom_.right), float(rectTo_.right), t)),
        static_cast<LONG>(Lerp(float(rectFrom_.bottom), float(rectTo_.bottom), t))};
    // interpolate around the moving centre so the window never leaves the work area
    r = WindowManager::ClampToWork(r, WindowManager::WorkAreaFor(rectTo_));
    wm_.SetRect(r);
    if (!tw.Active()) {
        wm_.SetRect(rectTo_);
        FinishTransition();
    }
}

void Application::FinishTransition() {
    screen_ = next_;
    transitioning_ = false;
    wm_.SetResizable(screen_ == Screen::Main);
    if (screen_ != Screen::Main) wm_.SetMinSize(0, 0);
    if (!startupMessage_.empty() && screen_ != Screen::Splash) {
        ui::Notify(tr(startupMessage_.c_str()), 2);
        startupMessage_.clear();
    }
}

void Application::FinishSetup() {
    if (transitioning_ || screen_ != Screen::Setup) return;
    settings_.firstLaunch = false;
    SaveSettings(false);
    ui::Notify(tr("Setup complete"), 1);
    BeginTransition(Screen::Main);
}

void Application::ReopenSetup() {
    if (transitioning_ || screen_ != Screen::Main) return;
    StoreWindowState();
    BeginTransition(Screen::Setup);
}

// ============================================================================ settings actions
Settings Application::SnapshotForSave() const {
    Settings out = settings_;
    profiles_.WriteBaseInto(out, settings_);
    return out;
}

bool Application::SaveSettings(bool notify) {
    StoreWindowState();
    Settings snap = SnapshotForSave();
    bool ok = config_.Save(snap);
    if (ok) lastSaved_ = ConfigManager::Serialize(snap);
    if (notify) ui::Notify(tr(ok ? "Settings saved" : "Could not save settings"), ok ? 1 : 2);
    return ok;
}

void Application::AutoSave(float dt) {
    autoSaveTimer_ += dt;
    if (autoSaveTimer_ < 3.f || transitioning_ || screen_ == Screen::Splash) return;
    autoSaveTimer_ = 0.f;
    StoreWindowState();
    Settings snap = SnapshotForSave();
    std::string cur = ConfigManager::Serialize(snap);
    if (cur != lastSaved_ && config_.Save(snap)) lastSaved_ = cur;
}

void Application::LoadSettings() {
    std::error_code ec;
    if (!std::filesystem::exists(config_.Path(), ec)) { ui::Notify(tr("Could not read that file"), 2); return; }
    Settings tmp;
    bool intact = config_.Load(tmp);
    std::string msg = config_.TakeMessage();
    tmp.firstLaunch = settings_.firstLaunch;
    tmp.hasWindowPos = settings_.hasWindowPos;
    tmp.windowX = settings_.windowX; tmp.windowY = settings_.windowY;
    tmp.windowW = settings_.windowW; tmp.windowH = settings_.windowH;
    settings_ = tmp;
    if (!intact && !msg.empty()) ui::Notify(tr(msg.c_str()), 2);
    else ui::Notify(tr("Settings loaded"), 1);
}

void Application::ResetFeatureSettings() {
    settings_.combat = CombatSettings();
    settings_.visuals = VisualSettings();
    settings_.movement = MovementSettings();
    settings_.misc = MiscSettings();
    ui::Notify(tr("Settings reset"), 1);
}

void Application::ResetAllSettings() {
    Settings d;
    d.firstLaunch = false;
    d.lastTab = settings_.lastTab;
    d.music = settings_.music;   // keep the playlist
    d.hasWindowPos = settings_.hasWindowPos;
    d.windowX = settings_.windowX; d.windowY = settings_.windowY;
    d.windowW = settings_.windowW; d.windowH = settings_.windowH;
    settings_ = d;
    ui::Notify(tr("Settings reset"), 1);
}

void Application::ExportSettings() {
    StoreWindowState();
    std::wstring path = wm_.FileDialog(true, L"hatenium-config.json");
    if (path.empty()) return;
    Settings snap = SnapshotForSave();
    bool ok = config_.Export(snap, std::filesystem::path(path));
    ui::Notify(tr(ok ? "Exported" : "Could not save settings"), ok ? 1 : 2);
}

void Application::ImportSettings() {
    std::wstring path = wm_.FileDialog(false, nullptr);
    if (path.empty()) return;
    Settings tmp;
    if (!config_.Import(tmp, std::filesystem::path(path))) { ui::Notify(tr("Could not read that file"), 2); return; }
    tmp.firstLaunch = settings_.firstLaunch;
    tmp.hasWindowPos = settings_.hasWindowPos;
    tmp.windowX = settings_.windowX; tmp.windowY = settings_.windowY;
    tmp.windowW = settings_.windowW; tmp.windowH = settings_.windowH;
    settings_ = tmp;
    ui::Notify(tr("Imported"), 1);
}

// ============================================================================ UI root
void Application::DrawUI() {
    const Theme& th = ThemeManager::Get().T();
    AnimationManager& am = AnimationManager::Get();
    ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::Begin("##hatenium_root", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // rounded window background + hairline border (the OS window region clips to the same radius)
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize();
    dl->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), Raw32(th.bg), th.windowRounding);
    dl->AddRect(ImVec2(p.x + 0.5f, p.y + 0.5f), ImVec2(p.x + sz.x - 0.5f, p.y + sz.y - 0.5f),
                Raw32(WithAlpha(th.accent, 0.35f)), th.windowRounding, 0, 1.f);

    // cross-fade: the old screen fades out in the first half, the new one in during the second half
    float alpha = 1.f;
    Screen show = screen_;
    if (transitioning_) {
        float t = am.Channel("stage").Progress();
        if (t < 0.5f) alpha = 1.f - t * 2.f;
        else { show = next_; alpha = t * 2.f - 1.f; }
        alpha = alpha * alpha * (3.f - 2.f * alpha);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, std::max(alpha, 0.001f));
    if (transitioning_) ImGui::BeginDisabled(true);   // input is ignored while the window animates
    switch (show) {
    case Screen::Splash: gui::DrawSplash(*this); break;
    case Screen::Setup:  gui::DrawSetup(*this); break;
    case Screen::Main:   gui::DrawMain(*this); break;
    }
    if (transitioning_) ImGui::EndDisabled();
    ImGui::PopStyleVar();

    ui::DrawNotifications(ImVec2(p.x + sz.x - S(14), p.y + S(show == Screen::Main ? 58.f : 14.f)));

    ImGui::End();
    ImGui::PopStyleVar(3);
}

} // namespace hat
