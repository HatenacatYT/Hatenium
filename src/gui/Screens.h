#pragma once
#include "core/Application.h"

namespace hat::gui {
void DrawSplash(Application& app);
void DrawSetup(Application& app);
void DrawMain(Application& app);

void DrawCombatPage(Application& app);
void DrawVisualsPage(Application& app);
void DrawHudPage(Application& app);
void DrawMovementPage(Application& app);
void DrawMusicPage(Application& app);
void DrawToolsPage(Application& app);
void DrawMiscPage(Application& app);
void DrawSettingsPage(Application& app);
} // namespace hat::gui
