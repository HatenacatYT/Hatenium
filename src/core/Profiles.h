#pragma once
#include <set>
#include <string>
#include "core/Settings.h"

namespace hat {

// Lower-case executable names of the running processes (read-only snapshot; nothing is opened or touched).
std::set<std::string> RunningProcessNames();

// Switches crosshair / HUD / keybinds automatically while a profile's game process is running.
class ProfileManager {
public:
    // Returns true when the active profile changed. `name` receives the new profile name ("" = back to the base setup).
    bool Update(float dt, Settings& s, std::string& name);
    const std::set<std::string>& Processes() const { return procs_; }
    int  ActiveIndex() const { return active_; }
    void Release(Settings& s);                  // call before saving / exiting so the base setup is restored
    static void CaptureInto(GameProfile& p, const Settings& s);
    // For saving: writes the base setup (and the edits made to the active profile) into `copy`, so a profile never leaks into the base config.
    void WriteBaseInto(Settings& copy, const Settings& live) const;

private:
    void Apply(const GameProfile& p, Settings& s);
    std::set<std::string> procs_;
    float timer_ = 100.f;
    int active_ = -1;
    struct Base { CrosshairSettings crosshair; HudSettings hud; int aimbotKey = 0, triggerKey = 0; } base_;
};

} // namespace hat
