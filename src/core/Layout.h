#pragma once
#include <filesystem>
#include <map>
#include <string>
#include "core/Settings.h"

namespace hat {

// layout.json: HUD element positions (in % of the monitor) remembered separately for every monitor resolution.
class LayoutStore {
public:
    void Load(const std::filesystem::path& dir);
    bool Save() const;
    bool Has(const std::string& monitorKey) const { return data_.count(monitorKey) != 0; }
    void Apply(const std::string& monitorKey, HudSettings& hud) const;
    void Store(const std::string& monitorKey, const HudSettings& hud);

private:
    struct Pos { float x = 0.f, y = 0.f; };
    struct Entry { Pos keys, cps, timer, watermark; };
    std::map<std::string, Entry> data_;
    std::filesystem::path file_;
};

} // namespace hat
