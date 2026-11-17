#pragma once
#include <filesystem>
#include <string>
#include "core/Settings.h"

namespace hat {

// JSON persistence with atomic writes (tmp file + replace), rolling .bak and corruption recovery.
class ConfigManager {
public:
    void Init();                                   // picks <exe>/config, falls back to %APPDATA%\Hatenium
    bool Load(Settings& out);                      // false = file was damaged (recovered or defaults used)
    bool Save(const Settings& s);
    bool Export(const Settings& s, const std::filesystem::path& file);
    bool Import(Settings& out, const std::filesystem::path& file);
    static std::string Serialize(const Settings& s);
    std::string TakeMessage() { std::string m; m.swap(message_); return m; }
    const std::filesystem::path& Path() const { return path_; }
    const std::filesystem::path& Dir() const { return dir_; }
    // Shareable presets: visuals + HUD only (no window state, music, profiles). Returns an error text, empty on success.
    std::string ExportPreset(const Settings& s, const std::filesystem::path& file);
    std::string ImportPreset(Settings& s, const std::filesystem::path& file);
private:
    std::filesystem::path dir_, path_;
    std::string message_;
};

void Sanitize(Settings& s);   // clamps every value into its valid range

} // namespace hat
