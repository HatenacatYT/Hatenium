#include "core/ToolsManager.h"
#include <cmath>
#include <cstdio>

namespace hat {

void ToolsManager::Update(float dt) {
    if (swRunning_) swElapsed_ += dt;
    if (tRunning_) {
        tRemaining_ -= dt;
        if (tRemaining_ <= 0.0) { tRemaining_ = 0.0; tRunning_ = false; finished_ = true; }
    }
}

void ToolsManager::TimerSet(int total) {
    if (tRunning_) return;
    tTotal_ = total < 0 ? 0 : total; tRemaining_ = tTotal_;
}

void ToolsManager::TimerStartPause() {
    if (tRemaining_ <= 0.0) tRemaining_ = tTotal_;
    if (tRemaining_ > 0.0) tRunning_ = !tRunning_;
}

void ToolsManager::TimerReset() { tRunning_ = false; tRemaining_ = tTotal_; }

std::string ToolsManager::Format(double s) {
    if (s < 0) s = 0;
    long long t = static_cast<long long>(std::floor(s));
    char b[32];
    if (t >= 3600) std::snprintf(b, sizeof(b), "%lld:%02lld:%02lld", t / 3600, (t / 60) % 60, t % 60);
    else std::snprintf(b, sizeof(b), "%02lld:%02lld", t / 60, t % 60);
    return b;
}

std::string ToolsManager::DisplayText() const {
    if (tRunning_ || (tTotal_ > 0.0 && tRemaining_ < tTotal_ && tRemaining_ > 0.0)) return Format(tRemaining_ + 0.999);
    if (swElapsed_ > 0.0 || swRunning_) return Format(swElapsed_);
    return Format(tTotal_ > 0.0 ? tRemaining_ : 0.0);
}

} // namespace hat
