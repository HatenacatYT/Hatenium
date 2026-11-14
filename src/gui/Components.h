#pragma once
#include <array>
#include <string>
#include "imgui.h"

namespace hat::ui {

// ---- controls (label on the left, control on the right; all animated) ----
bool Checkbox(const char* label, bool* v);
bool SliderFloat(const char* label, float* v, float mn, float mx, const char* fmt = "%.1f");
bool SliderInt(const char* label, int* v, int mn, int mx, const char* fmt = "%d");
bool Combo(const char* label, int* cur, const char* const* items, int count);
bool ColorField(const char* label, std::array<float, 3>& col);
bool Keybind(const char* label, int* key);
bool Button(const char* label, ImVec2 size = ImVec2(0, 0), bool primary = false);
// Selectable tile that previews a shape (kind 0: button, 1: window).
bool ShapeTile(const char* id, const char* label, bool selected, float rounding, int kind, float width);
bool Swatch(const char* id, const ImVec4& color, bool selected, float diameter);

// ---- layout ----
bool BeginCard(const char* title);
void EndCard();
bool BeginColumns(const char* id, int count);
void EndColumns();
int  ColumnCountForWidth(float logicalMinColumnWidth);
void PageHeader(const char* title, const char* description);
void NoteText(const char* text);

// ---- notifications (toasts) ----
void SetNotificationsEnabled(bool on);
void Notify(const std::string& text, int kind = 0);   // 0 info, 1 success, 2 warning
void DrawNotifications(ImVec2 areaTopRight);

} // namespace hat::ui
