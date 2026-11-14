#pragma once
#include <array>
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
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CrosshairSettings, enable, style, size, gap, thickness, color, opacity)

struct EspSettings {   // settings only - nothing is ever drawn or detected
    bool box = false;
    bool name = false;
    bool distance = false;
    bool snaplines = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EspSettings, box, name, distance, snaplines)

struct VisualSettings {
    FovCircleSettings fovCircle;
    CrosshairSettings crosshair;
    EspSettings esp;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VisualSettings, fovCircle, crosshair, esp)

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
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MiscSettings, watermark, animatedAccent, fpsCounter, notifications, uiAnimation, panelOpacity)

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

    bool  startupAnimation = true;
    float animationSpeed = 1.0f;
    int   language = 0;                 // 0 English, 1 Japanese
    int   lastTab = 0;

    CombatSettings   combat;
    VisualSettings   visuals;
    MovementSettings movement;
    MiscSettings     misc;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, version, firstLaunch, theme, customColor, buttonShape,
    windowAppearance, uiScale, hasWindowPos, windowX, windowY, windowW, windowH, rememberPosition, rememberSize,
    startupAnimation, animationSpeed, language, lastTab, combat, visuals, movement, misc)

} // namespace hat
