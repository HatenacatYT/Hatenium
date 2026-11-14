#include "core/AnimationManager.h"
#include <algorithm>

namespace hat {

float ApplyEase(Ease e, float t) {
    t = Clamp01(t);
    switch (e) {
    case Ease::Linear: return t;
    case Ease::OutCubic: { float u = 1.f - t; return 1.f - u * u * u; }
    case Ease::InOutCubic: return t < 0.5f ? 4.f * t * t * t : 1.f - std::pow(-2.f * t + 2.f, 3.f) * 0.5f;
    case Ease::OutBack: { const float c1 = 1.70158f, c3 = c1 + 1.f; float u = t - 1.f; return 1.f + c3 * u * u * u + c1 * u * u; }
    }
    return t;
}

void Tween::Start(float from, float to, float duration, Ease e) {
    from_ = from; to_ = to; ease_ = e; elapsed_ = 0.f;
    if (duration <= 0.f) { Snap(to); return; }
    dur_ = duration; t_ = 0.f; active_ = true;
}

void Tween::Snap(float v) { from_ = to_ = v; t_ = 1.f; elapsed_ = dur_; active_ = false; }

void Tween::Update(float dt) {
    if (!active_) return;
    elapsed_ += dt;
    t_ = dur_ > 0.f ? std::min(elapsed_ / dur_, 1.f) : 1.f;
    if (t_ >= 1.f) active_ = false;
}

float Tween::Value() const { return from_ + (to_ - from_) * ApplyEase(ease_, t_); }

AnimationManager& AnimationManager::Get() { static AnimationManager m; return m; }

void AnimationManager::Configure(bool enabled, float speed) {
    enabled_ = enabled;
    speed_ = std::min(std::max(speed, 0.1f), 5.f);
}

void AnimationManager::Update(float dt) {
    dt_ = std::min(std::max(dt, 0.f), 0.1f);
    time_ += dt_;
    for (auto& kv : channels_) kv.second.Update(dt_);
}

float AnimationManager::Smooth(unsigned id, float target, float rate) {
    auto it = values_.find(id);
    if (it == values_.end()) { values_[id] = target; return target; }
    if (!enabled_) { it->second = target; return target; }
    float k = 1.f - std::exp(-rate * speed_ * dt_);
    it->second += (target - it->second) * k;
    if (std::fabs(target - it->second) < 0.0005f) it->second = target;
    return it->second;
}

Tween& AnimationManager::Channel(const char* name) { return channels_[name]; }

} // namespace hat
