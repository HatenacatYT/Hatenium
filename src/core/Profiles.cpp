#include "core/Profiles.h"
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cctype>

namespace hat {

namespace {
std::string Lower(std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); }); return s; }
}

std::set<std::string> RunningProcessNames() {
    std::set<std::string> out;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return out;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            char buf[MAX_PATH * 3];
            int n = WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, buf, sizeof(buf), nullptr, nullptr);
            if (n > 1) out.insert(Lower(std::string(buf, static_cast<size_t>(n - 1))));
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return out;
}

void ProfileManager::CaptureInto(GameProfile& p, const Settings& s) {
    p.crosshair = s.visuals.crosshair; p.hud = s.hud;
    p.aimbotKey = s.combat.aimbot.key; p.triggerKey = s.combat.triggerbot.key;
}

void ProfileManager::WriteBaseInto(Settings& copy, const Settings& live) const {
    if (active_ < 0 || active_ >= static_cast<int>(copy.profiles.size())) return;
    CaptureInto(copy.profiles[static_cast<size_t>(active_)], live);
    copy.visuals.crosshair = base_.crosshair; copy.hud = base_.hud;
    copy.combat.aimbot.key = base_.aimbotKey; copy.combat.triggerbot.key = base_.triggerKey;
}

void ProfileManager::Apply(const GameProfile& p, Settings& s) {
    s.visuals.crosshair = p.crosshair; s.hud = p.hud;
    s.combat.aimbot.key = p.aimbotKey; s.combat.triggerbot.key = p.triggerKey;
}

void ProfileManager::Release(Settings& s) {
    if (active_ < 0) return;
    if (active_ < static_cast<int>(s.profiles.size())) CaptureInto(s.profiles[static_cast<size_t>(active_)], s);   // keep edits made while playing
    s.visuals.crosshair = base_.crosshair; s.hud = base_.hud;
    s.combat.aimbot.key = base_.aimbotKey; s.combat.triggerbot.key = base_.triggerKey;
    active_ = -1;
}

bool ProfileManager::Update(float dt, Settings& s, std::string& name) {
    timer_ += dt;
    if (timer_ < 2.f) return false;
    timer_ = 0.f;
    procs_ = RunningProcessNames();

    int match = -1;
    for (size_t i = 0; i < s.profiles.size(); ++i) {
        const auto& p = s.profiles[i];
        if (p.enabled && !p.exe.empty() && procs_.count(Lower(p.exe))) { match = static_cast<int>(i); break; }
    }
    if (active_ >= static_cast<int>(s.profiles.size())) active_ = -1;   // profile list changed under us
    if (match == active_) return false;

    if (active_ >= 0) Release(s);                      // store edits into the old profile, go back to the base setup
    if (match >= 0) {
        base_.crosshair = s.visuals.crosshair; base_.hud = s.hud;
        base_.aimbotKey = s.combat.aimbot.key; base_.triggerKey = s.combat.triggerbot.key;
        Apply(s.profiles[static_cast<size_t>(match)], s);
        active_ = match;
        name = s.profiles[static_cast<size_t>(match)].name;
    } else name.clear();
    return true;
}

} // namespace hat
