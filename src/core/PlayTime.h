#pragma once
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "core/Settings.h"

namespace hat {

// Records how long the tracked game processes are running (local playtime.json, nothing is sent anywhere).
class PlayTimeTracker {
public:
    struct Day { std::string label; double seconds = 0.0; };
    void Load(const std::filesystem::path& dir);
    void Update(float dt, const std::set<std::string>& runningLower, const ToolsSettings& t);
    void Flush();
    std::vector<Day> LastDays(int n) const;                          // oldest first, always n entries
    std::map<std::string, double> TotalsByGame(int days) const;
    bool Running() const { return running_; }

private:
    static std::string Today();
    bool Save() const;
    std::map<std::string, std::map<std::string, double>> data_;      // date -> exe -> seconds
    std::filesystem::path file_;
    float saveTimer_ = 0.f;
    bool dirty_ = false, running_ = false;
};

} // namespace hat
