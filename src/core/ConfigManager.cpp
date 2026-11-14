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
    s.lastTab = I(s.lastTab, 0, 4);

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

    s.movement.preset = I(s.movement.preset, 0, 3);
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

} // namespace hat
