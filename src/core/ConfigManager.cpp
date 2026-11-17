#include "core/ConfigManager.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;
using nlohmann::json;

namespace hat {

namespace {

fs::path ExeDir() {
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(sizeof(buf) / sizeof(buf[0])));
    return n ? fs::path(std::wstring(buf, n)).parent_path() : fs::current_path();
}

bool DirWritable(const fs::path& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    fs::path probe = dir / L".write_test";
    {
        std::ofstream f(probe, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << "x";
        if (!f) return false;
    }
    fs::remove(probe, ec);
    return true;
}

bool ParseFile(const fs::path& p, Settings& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    json j = json::parse(txt, nullptr, false);
    if (j.is_discarded() || !j.is_object()) return false;
    try { out = j.get<Settings>(); } catch (...) { return false; }
    Sanitize(out);
    return true;
}

// Writes to "<file>.tmp", flushes, then atomically replaces the target.
bool WriteAtomic(const fs::path& target, const std::string& text, bool keepBackup) {
    std::error_code ec;
    fs::path tmp = target; tmp += L".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f.write(text.data(), static_cast<std::streamsize>(text.size()));
        f.flush();
        if (!f) { f.close(); fs::remove(tmp, ec); return false; }
    }
    if (keepBackup && fs::exists(target, ec)) {
        fs::path bak = target; bak += L".bak";
        fs::copy_file(target, bak, fs::copy_options::overwrite_existing, ec);
    }
    if (!MoveFileExW(tmp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        fs::remove(tmp, ec);
        return false;
    }
    return true;
}

float F(float v, float lo, float hi, float fallback) {
    if (!std::isfinite(v)) return fallback;
    return std::min(std::max(v, lo), hi);
}
int I(int v, int lo, int hi) { return std::min(std::max(v, lo), hi); }
void C3(Col3& c) { for (float& x : c) x = F(x, 0.f, 1.f, 1.f); }

} // namespace

void Sanitize(Settings& s) {
    s.theme = I(s.theme, 0, 5);
    C3(s.customColor);
    s.buttonShape = I(s.buttonShape, 0, 3);
    s.windowAppearance = I(s.windowAppearance, 0, 3);
    s.uiScale = F(s.uiScale, 0.75f, 2.0f, 1.f);
    s.windowW = (s.windowW > 0) ? I(s.windowW, 400, 8000) : 0;
    s.windowH = (s.windowH > 0) ? I(s.windowH, 300, 8000) : 0;
    s.windowX = I(s.windowX, -32000, 32000);
    s.windowY = I(s.windowY, -32000, 32000);
    s.animationSpeed = F(s.animationSpeed, 0.25f, 3.f, 1.f);
    s.language = I(s.language, 0, 1);
    s.lastTab = I(s.lastTab, 0, 7);

    auto& a = s.combat.aimbot;
    a.fov = F(a.fov, 1.f, 180.f, 90.f); a.smoothness = F(a.smoothness, 1.f, 30.f, 8.f); a.target = I(a.target, 0, 3);
    auto& t = s.combat.triggerbot;
    t.delayMs = F(t.delayMs, 0.f, 500.f, 30.f); t.hitChance = F(t.hitChance, 0.f, 100.f, 100.f);
    auto& r = s.combat.recoil;
    r.horizontal = F(r.horizontal, 0.f, 100.f, 50.f); r.vertical = F(r.vertical, 0.f, 100.f, 50.f);

    auto& f = s.visuals.fovCircle;
    f.radius = F(f.radius, 10.f, 300.f, 120.f); f.thickness = F(f.thickness, 0.5f, 6.f, 1.5f);
    f.opacity = F(f.opacity, 0.f, 1.f, 0.9f); f.centerX = F(f.centerX, -100.f, 100.f, 0.f);
    f.centerY = F(f.centerY, -100.f, 100.f, 0.f); C3(f.color);
    auto& c = s.visuals.crosshair;
    c.style = I(c.style, 0, 3); c.size = F(c.size, 2.f, 40.f, 10.f); c.gap = F(c.gap, 0.f, 20.f, 4.f);
    c.thickness = F(c.thickness, 0.5f, 6.f, 1.5f); c.opacity = F(c.opacity, 0.f, 1.f, 1.f); C3(c.color);

    c.animAmount = F(c.animAmount, 0.f, 40.f, 8.f); c.animSpeed = F(c.animSpeed, 1.f, 40.f, 12.f); c.easing = I(c.easing, 0, 1);
    auto& ko = s.hud.keys;
    ko.scale = F(ko.scale, 0.5f, 3.f, 1.f); ko.posX = F(ko.posX, 0.f, 100.f, 2.f); ko.posY = F(ko.posY, 0.f, 100.f, 55.f);
    ko.opacity = F(ko.opacity, 0.1f, 1.f, 0.9f); C3(ko.activeColor); C3(ko.inactiveColor); C3(ko.textColor);
    auto& cp = s.hud.cps;
    cp.scale = F(cp.scale, 0.5f, 4.f, 1.f); cp.posX = F(cp.posX, 0.f, 100.f, 2.f); cp.posY = F(cp.posY, 0.f, 100.f, 42.f);
    cp.opacity = F(cp.opacity, 0.1f, 1.f, 0.95f); C3(cp.color);
    s.visuals.overlay.monitor = (s.visuals.overlay.monitor == 100) ? 100 : I(s.visuals.overlay.monitor, 0, 16);
    auto& tr = s.visuals.trail;
    tr.style = I(tr.style, 0, 2); tr.colorMode = I(tr.colorMode, 0, 2); C3(tr.color); C3(tr.color2);
    tr.thickness = F(tr.thickness, 1.f, 30.f, 4.f); tr.lifetimeMs = F(tr.lifetimeMs, 50.f, 2000.f, 450.f);
    tr.opacity = F(tr.opacity, 0.f, 1.f, 0.9f); tr.rainbowSpeed = F(tr.rainbowSpeed, 0.1f, 5.f, 1.f);
    auto& pc = s.visuals.particles;
    pc.count = I(pc.count, 1, 80); pc.shape = I(pc.shape, 0, 3); pc.colorMode = I(pc.colorMode, 0, 3);
    C3(pc.color); C3(pc.color2);
    pc.size = F(pc.size, 1.f, 40.f, 6.f); pc.speed = F(pc.speed, 20.f, 1200.f, 260.f);
    pc.lifetimeMs = F(pc.lifetimeMs, 100.f, 3000.f, 700.f); pc.gravity = F(pc.gravity, -1000.f, 2000.f, 300.f);
    pc.opacity = F(pc.opacity, 0.f, 1.f, 1.f);

    auto& wmk = s.hud.watermark;
    wmk.posX = F(wmk.posX, 0.f, 100.f, 84.f); wmk.posY = F(wmk.posY, 0.f, 100.f, 3.f);
    wmk.scale = F(wmk.scale, 0.05f, 5.f, 1.f); wmk.opacity = F(wmk.opacity, 0.f, 1.f, 0.8f);
    auto& th = s.hud.timer;
    th.posX = F(th.posX, 0.f, 100.f, 44.f); th.posY = F(th.posY, 0.f, 100.f, 10.f); th.scale = F(th.scale, 0.5f, 4.f, 1.f);
    C3(s.hud.background); s.hud.borderThickness = F(s.hud.borderThickness, 0.f, 6.f, 1.5f);
    s.tools.timerMinutes = I(s.tools.timerMinutes, 0, 999); s.tools.timerSeconds = I(s.tools.timerSeconds, 0, 59);
    if (s.tools.trackedProcesses.size() > 20) s.tools.trackedProcesses.resize(20);
    if (s.profiles.size() > 50) s.profiles.resize(50);
    for (auto& p : s.profiles) {
        p.crosshair.style = I(p.crosshair.style, 0, 3);
        p.crosshair.size = F(p.crosshair.size, 2.f, 40.f, 10.f); p.crosshair.gap = F(p.crosshair.gap, 0.f, 20.f, 4.f);
        p.crosshair.thickness = F(p.crosshair.thickness, 0.5f, 6.f, 1.5f); p.crosshair.opacity = F(p.crosshair.opacity, 0.f, 1.f, 1.f);
        C3(p.crosshair.color);
        p.crosshair.animAmount = F(p.crosshair.animAmount, 0.f, 40.f, 8.f); p.crosshair.animSpeed = F(p.crosshair.animSpeed, 1.f, 40.f, 12.f);
        p.crosshair.easing = I(p.crosshair.easing, 0, 1);
    }
    s.movement.preset = I(s.movement.preset, 0, 3);
    s.music.volume = F(s.music.volume, 0.f, 1.f, 0.7f);
    s.music.repeat = I(s.music.repeat, 0, 2);
    s.music.source = I(s.music.source, 0, 1);
    if (s.music.tracks.size() > 5000) s.music.tracks.resize(5000);
    if (s.music.current >= static_cast<int>(s.music.tracks.size())) s.music.current = -1;
    s.misc.panelOpacity = F(s.misc.panelOpacity, 0.3f, 1.f, 0.92f);
}

void ConfigManager::Init() {
    std::error_code ec;
    fs::path dir = ExeDir() / L"config";
    if (!DirWritable(dir)) {
        wchar_t* ap = _wgetenv(L"APPDATA");
        if (ap) dir = fs::path(ap) / L"Hatenium";
        fs::create_directories(dir, ec);
    }
    dir_ = dir;
    path_ = dir_ / L"hatenium.json";
}

std::string ConfigManager::Serialize(const Settings& s) {
    json j = s;
    return j.dump(2);
}

bool ConfigManager::Load(Settings& out) {
    std::error_code ec;
    message_.clear();
    if (!fs::exists(path_, ec)) { out = Settings(); return true; }   // first run

    Settings tmp;
    if (ParseFile(path_, tmp)) { out = tmp; return true; }

    // ---- damaged file: quarantine it, try the backup, else defaults ----
    fs::path corrupt = path_; corrupt += L".corrupt";
    fs::remove(corrupt, ec);
    fs::rename(path_, corrupt, ec);

    fs::path bak = path_; bak += L".bak";
    Settings b;
    if (fs::exists(bak, ec) && ParseFile(bak, b)) {
        out = b;
        message_ = "Config file was damaged - restored from backup";
        Save(out);
        return false;
    }
    out = Settings();
    message_ = "Config file was damaged - defaults loaded";
    return false;
}

bool ConfigManager::Save(const Settings& s) { return WriteAtomic(path_, Serialize(s), true); }

bool ConfigManager::Export(const Settings& s, const fs::path& file) { return WriteAtomic(file, Serialize(s), false); }

bool ConfigManager::Import(Settings& out, const fs::path& file) {
    Settings tmp;
    if (!ParseFile(file, tmp)) return false;
    out = tmp;
    return true;
}

std::string ConfigManager::ExportPreset(const Settings& s, const fs::path& file) {
    json j;
    j["format"] = "hatenium-preset";
    j["version"] = 1;
    j["app"] = "Hatenium";
    j["visuals"] = s.visuals;
    j["hud"] = s.hud;
    return WriteAtomic(file, j.dump(2), false) ? std::string() : std::string("Could not write the preset file");
}

std::string ConfigManager::ImportPreset(Settings& s, const fs::path& file) {
    std::ifstream f(file, std::ios::binary);
    if (!f) return "Could not open the preset file";
    std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    json j = json::parse(txt, nullptr, false);
    if (j.is_discarded() || !j.is_object()) return "The file is not valid JSON";
    if (j.value("format", std::string()) != "hatenium-preset") return "This is not a Hatenium preset";
    if (j.value("version", 0) < 1) return "Unknown preset version";
    Settings tmp = s;
    try {
        if (j.contains("visuals")) tmp.visuals = j["visuals"].get<VisualSettings>();   // missing fields fall back to defaults
        if (j.contains("hud")) tmp.hud = j["hud"].get<HudSettings>();
    } catch (...) { return "The preset contains invalid values"; }
    tmp.visuals.overlay = s.visuals.overlay;       // a preset never switches the overlay on or off by itself
    Sanitize(tmp);
    s = tmp;
    return std::string();
}

} // namespace hat
