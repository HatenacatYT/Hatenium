#pragma once
#include <array>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace hat {

using Col3 = std::array<float, 3>;

// ---- Combat -------------------------------------------------------------
struct AimbotSettings {
    bool  enable = false;
    float fov = 90.f;
    float smoothness = 8.f;
    int   target = 0;      // 0 Closest, 1 Head, 2 Chest, 3 Body
    int   key = 0;         // 0 = none, 1000+n = mouse button n, else ImGuiKey
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AimbotSettings, enable, fov, smoothness, target, key)

struct TriggerSettings {
    bool  enable = false;
    int   key = 0;
    float delayMs = 30.f;
    float hitChance = 100.f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriggerSettings, enable, key, delayMs, hitChance)

struct RecoilSettings {
    bool  enable = false;
    float horizontal = 50.f;
    float vertical = 50.f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RecoilSettings, enable, horizontal, vertical)

struct OtherCombatSettings {
    bool noRecoil = false;
    bool noSpread = false;
    bool instantReload = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OtherCombatSettings, noRecoil, noSpread, instantReload)

struct CombatSettings {
    AimbotSettings aimbot;
    TriggerSettings triggerbot;
    RecoilSettings recoil;
    OtherCombatSettings other;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CombatSettings, aimbot, triggerbot, recoil, other)

// ---- Visuals ------------------------------------------------------------
struct FovCircleSettings {
    bool  enable = true;
    float radius = 120.f;
    float thickness = 1.5f;
    float opacity = 0.9f;
    Col3  color{0.0f, 0.9f, 1.0f};
    bool  filled = false;
    bool  outline = true;
    float centerX = 0.f;   // -100..100 (% of half preview width)
    float centerY = 0.f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FovCircleSettings, enable, radius, thickness, opacity, color, filled, outline, centerX, centerY)

struct CrosshairSettings {
    bool  enable = true;
    int   style = 0;       // 0 Cross, 1 Dot, 2 Circle, 3 Cross + Dot
    float size = 10.f;
    float gap = 4.f;
    float thickness = 1.5f;
    Col3  color{1.f, 1.f, 1.f};
    float opacity = 1.f;
    bool  spreadOnShoot = false;   // gap widens while the left mouse button is held
    bool  shrinkOnMove = false;    // gap narrows while W/A/S/D is held
    float animAmount = 8.f;        // px
    float animSpeed = 12.f;        // higher = faster
    int   easing = 1;              // 0 Linear, 1 Ease Out
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CrosshairSettings, enable, style, size, gap, thickness, color, opacity,
    spreadOnShoot, shrinkOnMove, animAmount, animSpeed, easing)

struct EspSettings {   // settings only - nothing is ever drawn or detected
    bool box = false;
    bool name = false;
    bool distance = false;
    bool snaplines = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EspSettings, box, name, distance, snaplines)

struct OverlaySettings {   // real on-screen overlay (transparent, click-through window drawn by Hatenium itself)
    bool enable = false;
    int  monitor = 0;      // 0 primary, 1..n = nth monitor, 100 = all monitors
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OverlaySettings, enable, monitor)

struct MouseTrailSettings {
    bool  enable = false;
    int   style = 0;          // 0 Line, 1 Dots, 2 Comet
    int   colorMode = 0;      // 0 Solid, 1 Gradient, 2 Rainbow
    Col3  color{0.0f, 0.9f, 1.0f};
    Col3  color2{0.58f, 0.38f, 1.0f};
    float thickness = 4.f;    // px
    float lifetimeMs = 450.f;
    float opacity = 0.9f;
    bool  taper = true;
    bool  glow = true;
    float rainbowSpeed = 1.f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MouseTrailSettings, enable, style, colorMode, color, color2, thickness,
    lifetimeMs, opacity, taper, glow, rainbowSpeed)

struct ClickParticleSettings {
    bool  enable = false;
    bool  onLeft = true;
    bool  onRight = false;
    int   count = 18;
    int   shape = 0;          // 0 Circle, 1 Square, 2 Star, 3 Ring
    int   colorMode = 0;      // 0 Solid, 1 Gradient, 2 Rainbow, 3 Random
    Col3  color{0.0f, 0.9f, 1.0f};
    Col3  color2{1.0f, 0.3f, 0.6f};
    float size = 6.f;         // px
    float speed = 260.f;      // px/s
    float lifetimeMs = 700.f;
    float gravity = 300.f;    // px/s^2 (0 = none)
    float opacity = 1.f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ClickParticleSettings, enable, onLeft, onRight, count, shape, colorMode,
    color, color2, size, speed, lifetimeMs, gravity, opacity)

struct VisualSettings {
    FovCircleSettings fovCircle;
    CrosshairSettings crosshair;
    EspSettings esp;
    OverlaySettings overlay;
    MouseTrailSettings trail;
    ClickParticleSettings particles;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VisualSettings, fovCircle, crosshair, esp, overlay, trail, particles)

// ---- HUD (drawn by the overlay) -------------------------------------------
struct KeyOverlaySettings {
    bool  enable = false;
    bool  w = true, a = true, s = true, d = true, space = true, shift = true, ctrl = true, lmb = true, rmb = true;
    bool  r = false, e = false, q = false, f = false, side1 = false, side2 = false;
    float scale = 1.f;
    float posX = 2.f, posY = 55.f;                 // % of the monitor (top-left of the block)
    Col3  activeColor{0.0f, 0.9f, 1.0f};
    Col3  inactiveColor{0.08f, 0.10f, 0.15f};
    Col3  textColor{0.9f, 0.95f, 1.0f};
    float opacity = 0.9f;
    bool  showLabels = true;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KeyOverlaySettings, enable, w, a, s, d, space, shift, ctrl, lmb, rmb,
    r, e, q, f, side1, side2, scale, posX, posY, activeColor, inactiveColor, textColor, opacity, showLabels)

struct CpsSettings {
    bool  enable = false;
    bool  showMax = true;
    float scale = 1.f;
    float posX = 2.f, posY = 42.f;
    Col3  color{0.0f, 0.9f, 1.0f};
    float opacity = 0.95f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CpsSettings, enable, showMax, scale, posX, posY, color, opacity)

struct WatermarkSettings {          // streamer watermark: a PNG/JPEG shown on the overlay
    bool  enable = false;
    std::string path;               // UTF-8
    float posX = 84.f, posY = 3.f;  // % of the monitor
    float scale = 1.f;              // relative to the image's pixel size
    float opacity = 0.8f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WatermarkSettings, enable, path, posX, posY, scale, opacity)

struct TimerHudSettings {           // shows the running timer / stopwatch on the overlay
    bool  enable = false;
    float posX = 44.f, posY = 10.f;
    float scale = 1.f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TimerHudSettings, enable, posX, posY, scale)

struct HudSettings {
    KeyOverlaySettings keys;
    CpsSettings cps;
    WatermarkSettings watermark;
    TimerHudSettings timer;
    Col3  background{0.04f, 0.06f, 0.10f};
    float borderThickness = 1.5f;
    std::string theme = "Cyberpunk";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HudSettings, keys, cps, watermark, timer, background, borderThickness, theme)

struct GameProfile {                // switched automatically while the process is running
    std::string name;
    std::string exe;                // e.g. RobloxPlayerBeta.exe
    bool enabled = true;
    CrosshairSettings crosshair;
    HudSettings hud;
    int aimbotKey = 0;
    int triggerKey = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GameProfile, name, exe, enabled, crosshair, hud, aimbotKey, triggerKey)

struct ToolsSettings {
    int  timerMinutes = 10;
    int  timerSeconds = 0;
    bool trackPlaytime = true;
    bool onlineIpLookup = true;        // look the Roblox server IP up online (only that IP is sent)
    std::vector<std::string> trackedProcesses{"RobloxPlayerBeta.exe"};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToolsSettings, timerMinutes, timerSeconds, trackPlaytime, onlineIpLookup, trackedProcesses)

// ---- Movement / Misc ----------------------------------------------------
struct MovementSettings {
    bool bunnyHop = false;
    bool autoStrafe = false;
    bool speedIndicator = true;
    int  preset = 0;       // 0 Default, 1 Smooth, 2 Aggressive, 3 Custom
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MovementSettings, bunnyHop, autoStrafe, speedIndicator, preset)

struct MiscSettings {
    bool  watermark = true;
    bool  animatedAccent = true;
    bool  fpsCounter = true;
    bool  notifications = true;
    bool  uiAnimation = true;
    float panelOpacity = 0.92f;
    bool  preventSleep = false;    // keeps Windows/display awake (SetThreadExecutionState); no fake input is ever sent
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MiscSettings, watermark, animatedAccent, fpsCounter, notifications, uiAnimation, panelOpacity, preventSleep)

struct MusicSettings {
    float volume = 0.7f;
    bool  shuffle = false;
    int   repeat = 0;                 // 0 off, 1 all, 2 one
    int   current = -1;
    int   source = 0;                 // 0 Local, 1 YouTube
    std::vector<std::string> tracks;  // UTF-8 file paths
    std::string youtubeHome = "https://music.youtube.com";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MusicSettings, volume, shuffle, repeat, current, source, tracks, youtubeHome)

// ---- Root ---------------------------------------------------------------
struct Settings {
    int   version = 1;
    bool  firstLaunch = true;

    int   theme = 0;                    // 0 Cyan 1 Purple 2 Red 3 Green 4 White 5 Custom
    Col3  customColor{0.0f, 0.9f, 1.0f};
    int   buttonShape = 1;              // 0 Square 1 Slight 2 Rounded 3 Pill
    int   windowAppearance = 2;         // 0 Sharp 1 Soft 2 Rounded 3 Extra
    float uiScale = 1.0f;

    bool  hasWindowPos = false;
    int   windowX = 0, windowY = 0;     // physical pixels
    int   windowW = 0, windowH = 0;     // logical pixels (DPI independent)
    bool  rememberPosition = true;
    bool  rememberSize = true;
    bool  alwaysOnTop = false;          // keep the Hatenium window above other windows

    bool  startupAnimation = true;
    float animationSpeed = 1.0f;
    int   language = 0;                 // 0 English, 1 Japanese
    int   lastTab = 0;

    CombatSettings   combat;
    VisualSettings   visuals;
    HudSettings      hud;
    MovementSettings movement;
    MiscSettings     misc;
    MusicSettings    music;
    ToolsSettings    tools;
    std::vector<GameProfile> profiles;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, version, firstLaunch, theme, customColor, buttonShape,
    windowAppearance, uiScale, hasWindowPos, windowX, windowY, windowW, windowH, rememberPosition, rememberSize, alwaysOnTop,
    startupAnimation, animationSpeed, language, lastTab, combat, visuals, hud, movement, misc, music, tools, profiles)

} // namespace hat
