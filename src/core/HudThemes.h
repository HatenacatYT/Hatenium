#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "core/Settings.h"

namespace hat {

struct HudTheme {
    std::string name;
    Col3  background{0.04f, 0.06f, 0.10f};
    Col3  text{0.9f, 0.95f, 1.0f};
    Col3  accent{0.0f, 0.9f, 1.0f};
    float border = 1.5f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HudTheme, name, background, text, accent, border)

// Built-in presets + user themes stored in themes.json (and importable / exportable as single-theme files).
class HudThemes {
public:
    void Load(const std::filesystem::path& dir);
    bool Save() const;
    int  Count() const { return static_cast<int>(builtin_.size() + user_.size()); }
    const HudTheme& At(int i) const { return i < static_cast<int>(builtin_.size()) ? builtin_[static_cast<size_t>(i)] : user_[static_cast<size_t>(i) - builtin_.size()]; }
    bool IsBuiltin(int i) const { return i < static_cast<int>(builtin_.size()); }
    int  IndexOf(const std::string& name) const;
    void AddOrReplaceUser(const HudTheme& t);
    void RemoveUser(int index);
    std::string Export(int index, const std::filesystem::path& file) const;
    std::string Import(const std::filesystem::path& file);          // returns an error text, empty on success
    static void Apply(const HudTheme& t, HudSettings& hud);
    static HudTheme FromHud(const HudSettings& hud, const std::string& name);

private:
    std::vector<HudTheme> builtin_, user_;
    std::filesystem::path file_;
};

} // namespace hat
