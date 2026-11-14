#pragma once
#include <cmath>
#include <string>
#include <unordered_map>

namespace hat {

enum class Ease { Linear, OutCubic, InOutCubic, OutBack };

inline float Clamp01(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
float ApplyEase(Ease e, float t);

// A timed interpolation. Restarting it at any moment is always safe.
class Tween {
public:
    void  Start(float from, float to, float duration, Ease e = Ease::InOutCubic);
    void  Snap(float v);
    void  Update(float dt);
    float Value() const;                       // eased value
    float Progress() const { return t_; }      // raw 0..1
    bool  Active() const { return active_; }
private:
    float from_ = 1.f, to_ = 1.f, dur_ = 0.f, elapsed_ = 0.f, t_ = 1.f;
    Ease  ease_ = Ease::InOutCubic;
    bool  active_ = false;
};

// Central place for every animation: per-widget smoothing, named tweens, global speed/enable switch.
class AnimationManager {
public:
    static AnimationManager& Get();

    void  Update(float dt);
    void  Configure(bool enabled, float speed);
    bool  Enabled() const { return enabled_; }
    float Speed() const { return speed_; }
    float Duration(float base) const { return enabled_ ? base / speed_ : 0.f; }
    float DeltaTime() const { return dt_; }
    float Time() const { return time_; }

    // Exponentially smoothed value keyed by an id (hover, toggle, slider ...). First call snaps to target.
    float Smooth(unsigned id, float target, float rate = 14.f);
    // Named, long-lived tween (stage transition, tab transition ...).
    Tween& Channel(const char* name);

private:
    bool  enabled_ = true;
    float speed_ = 1.f, dt_ = 0.016f, time_ = 0.f;
    std::unordered_map<unsigned, float> values_;
    std::unordered_map<std::string, Tween> channels_;
};

} // namespace hat
