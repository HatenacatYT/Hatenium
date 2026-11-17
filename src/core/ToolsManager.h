#pragma once
#include <string>

namespace hat {

// Countdown timer + stopwatch (pure logic, drawn by the Tools page and optionally by the overlay).
class ToolsManager {
public:
    void Update(float dt);
    // stopwatch
    void StopwatchStartStop() { swRunning_ = !swRunning_; }
    void StopwatchReset() { swRunning_ = false; swElapsed_ = 0.0; }
    bool StopwatchRunning() const { return swRunning_; }
    double StopwatchElapsed() const { return swElapsed_; }
    // timer
    void TimerSet(int totalSeconds);
    void TimerStartPause();
    void TimerReset();
    bool TimerRunning() const { return tRunning_; }
    double TimerRemaining() const { return tRemaining_; }
    double TimerTotal() const { return tTotal_; }
    bool ConsumeTimerFinished() { bool f = finished_; finished_ = false; return f; }
    // text for the overlay: timer if it is active, else stopwatch
    std::string DisplayText() const;
    static std::string Format(double seconds);

private:
    bool swRunning_ = false, tRunning_ = false, finished_ = false;
    double swElapsed_ = 0.0, tTotal_ = 0.0, tRemaining_ = 0.0;
};

} // namespace hat
