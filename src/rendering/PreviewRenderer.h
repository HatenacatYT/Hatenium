#pragma once
#include "imgui.h"
#include "core/Settings.h"

namespace hat {

// Draws the Visuals preview canvas. Everything is rendered with the ImGui draw list inside Hatenium's own window -
// nothing is ever drawn over another program.
void DrawVisualPreview(ImDrawList* dl, ImVec2 min, ImVec2 max, const VisualSettings& v, float time);

// Small animated speed read-out used by the Movement page (simulated values).
void DrawSpeedPreview(ImDrawList* dl, ImVec2 min, ImVec2 max, float time);

} // namespace hat
