#pragma once
#include <windows.h>
#include <string>
#include "core/ConfigManager.h"
#include "core/Settings.h"
#include "core/WindowManager.h"
#include "rendering/Renderer.h"

struct ImFont;

namespace hat {

enum class Screen { Splash, Setup, Main };

class Application {
public:
    int Run(HINSTANCE inst);

    // ---- state used by the GUI ----
    Settings& settings() { return settings_; }
    WindowManager& window() { return wm_; }
    Screen CurrentScreen() const { return screen_; }
    bool Busy() const { return transitioning_; }          // a screen transition is running: input is ignored
    float Fps() const { return fps_; }
    float SplashProgress() const { return splashBar_; }
    const char* SplashStatus() const { return splashStatus_; }

    // ---- actions ----
    void FinishSetup();
    void ReopenSetup();
    void RequestClose() { wm_.RequestClose(); }
    void ResizeMainWindow(int logicalW, int logicalH);
    void MaxLogicalWindowSize(int& w, int& h) const;
    bool SaveSettings(bool notify);
    void LoadSettings();
    void ResetFeatureSettings();
    void ResetAllSettings();
    void ExportSettings();
    void ImportSettings();

private:
    bool InitGraphics(HINSTANCE inst);
    void Shutdown();
    void RebuildFonts();
    void SyncScaleAndLanguage();
    void HandleWindowEvents();
    void UpdateSplashTasks(float dt);
    void UpdateTransition();
    void BeginTransition(Screen next);
    void FinishTransition();
    void StoreWindowState();
    void KeepOnScreen();
    void EnforceMinSize();
    void AutoSave(float dt);
    void DrawUI();
    RECT RectFor(Screen s, const RECT& around) const;
    int  MinMainW() const;
    int  MinMainH() const;

    Settings settings_;
    ConfigManager config_;
    WindowManager wm_;
    Renderer renderer_;

    Screen screen_ = Screen::Splash, next_ = Screen::Splash;
    bool transitioning_ = false;
    RECT rectFrom_{}, rectTo_{};

    float scale_ = 1.f, fps_ = 60.f;
    bool fontsDirty_ = true, imguiReady_ = false;
    int  fontsLanguage_ = -1;

    // splash
    float splashTime_ = 0.f, splashBar_ = 0.f;
    int   initStep_ = 0;
    const char* splashStatus_ = "Loading configuration";
    bool  splashDone_ = false;

    std::string startupMessage_;
    std::string lastSaved_;
    float autoSaveTimer_ = 0.f;
};

} // namespace hat
