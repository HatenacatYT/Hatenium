#include "core/HudThemes.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <windows.h>

namespace fs = std::filesystem;
using nlohmann::json;

namespace hat {

namespace {
Col3 Clamp3(Col3 c) { for (float& v : c) v = std::min(std::max(v, 0.f), 1.f); return c; }
std::string ReadAll(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
bool WriteFileAtomic(const fs::path& target, const std::string& text) {
    fs::path tmp = target; tmp += L".tmp";
    { std::ofstream f(tmp, std::ios::binary | std::ios::trunc); if (!f) return false; f << text; f.flush(); if (!f) return false; }
    return MoveFileExW(tmp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}
} // namespace

void HudThemes::Load(const fs::path& dir) {
    file_ = dir / L"themes.json";
    builtin_ = {
        {"Cyberpunk", {0.04f, 0.06f, 0.12f}, {0.85f, 1.00f, 1.00f}, {0.00f, 0.95f, 1.00f}, 1.5f},
        {"Minimal",   {0.07f, 0.07f, 0.07f}, {1.00f, 1.00f, 1.00f}, {1.00f, 1.00f, 1.00f}, 1.0f},
        {"Gaming",    {0.10f, 0.02f, 0.03f}, {1.00f, 0.90f, 0.90f}, {1.00f, 0.15f, 0.20f}, 2.0f},
        {"Sunset",    {0.10f, 0.05f, 0.09f}, {1.00f, 0.93f, 0.85f}, {1.00f, 0.55f, 0.20f}, 1.5f},
    };
    user_.clear();
    json j = json::parse(ReadAll(file_), nullptr, false);
    if (j.is_object() && j.contains("themes") && j["themes"].is_array()) {
        for (auto& e : j["themes"]) {
            try {
                HudTheme t = e.get<HudTheme>();
                if (t.name.empty() || IndexOf(t.name) >= 0) continue;
                t.background = Clamp3(t.background); t.text = Clamp3(t.text); t.accent = Clamp3(t.accent);
                t.border = std::min(std::max(t.border, 0.f), 6.f);
                user_.push_back(t);
            } catch (...) {}
        }
    }
}

bool HudThemes::Save() const {
    json j;
    j["version"] = 1;
    j["themes"] = user_;
    return WriteFileAtomic(file_, j.dump(2));
}

int HudThemes::IndexOf(const std::string& name) const {
    for (int i = 0; i < Count(); ++i) if (At(i).name == name) return i;
    return -1;
}

void HudThemes::AddOrReplaceUser(const HudTheme& t) {
    for (auto& u : user_) if (u.name == t.name) { u = t; return; }
    if (user_.size() < 100) user_.push_back(t);
}

void HudThemes::RemoveUser(int index) {
    int u = index - static_cast<int>(builtin_.size());
    if (u >= 0 && u < static_cast<int>(user_.size())) user_.erase(user_.begin() + u);
}

std::string HudThemes::Export(int index, const fs::path& file) const {
    if (index < 0 || index >= Count()) return "No theme selected";
    json j; j["format"] = "hatenium-theme"; j["version"] = 1; j["theme"] = At(index);
    return WriteFileAtomic(file, j.dump(2)) ? std::string() : std::string("Could not write the theme file");
}

std::string HudThemes::Import(const fs::path& file) {
    json j = json::parse(ReadAll(file), nullptr, false);
    if (j.is_discarded() || !j.is_object()) return "The file is not valid JSON";
    if (j.value("format", std::string()) != "hatenium-theme" || j.value("version", 0) < 1 || !j.contains("theme")) return "This is not a Hatenium theme";
    HudTheme t;
    try { t = j["theme"].get<HudTheme>(); } catch (...) { return "The theme contains invalid values"; }
    if (t.name.empty() || t.name.size() > 40) return "The theme needs a name (max 40 characters)";
    if (IndexOf(t.name) >= 0 && IsBuiltin(IndexOf(t.name))) t.name += " (imported)";
    t.background = Clamp3(t.background); t.text = Clamp3(t.text); t.accent = Clamp3(t.accent);
    t.border = std::min(std::max(t.border, 0.f), 6.f);
    AddOrReplaceUser(t);
    return std::string();
}

void HudThemes::Apply(const HudTheme& t, HudSettings& hud) {
    hud.theme = t.name;
    hud.background = t.background;
    hud.borderThickness = t.border;
    hud.keys.activeColor = t.accent; hud.keys.inactiveColor = t.background; hud.keys.textColor = t.text;
    hud.cps.color = t.accent;
}

HudTheme HudThemes::FromHud(const HudSettings& hud, const std::string& name) {
    HudTheme t;
    t.name = name; t.background = hud.background; t.accent = hud.keys.activeColor; t.text = hud.keys.textColor; t.border = hud.borderThickness;
    return t;
}

} // namespace hat
