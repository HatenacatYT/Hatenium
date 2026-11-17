#pragma once
#include <deque>
#include <string>
#include <vector>
#include "core/Settings.h"
#include "rendering/Effects.h"

namespace hat {

// Key-stroke display and CPS counter. Inputs are read with GetAsyncKeyState (polling only - no hooks, nothing is recorded).
class HudRenderer {
public:
    void Update(float dt, const HudSettings& s);
    // origin/size describe the monitor rectangle in overlay coordinates
    void Render(DrawSink& sink, const HudSettings& s, Vec2 origin, Vec2 size, const std::string& timerText) const;
    // Rectangles of the enabled elements (ids: 0 keys, 1 cps, 2 timer) - used by the layout editor.
    struct Box { int id; Vec2 a, b; };
    std::vector<Box> Bounds(const HudSettings& s, Vec2 origin, Vec2 size) const;
    bool Active(const HudSettings& s) const { return s.keys.enable || s.cps.enable; }
    void ResetCps();
    bool KeyDown(int index) const;

private:
    float keyAnim_[15] = {};
    bool prevLmb_ = false;
    float clock_ = 0.f;
    std::deque<float> clicks_;
    int cps_ = 0, maxCps_ = 0;
};

// Set from the UI (any thread-free caller); consumed by the next HudRenderer::Update.
void RequestCpsReset();

} // namespace hat
