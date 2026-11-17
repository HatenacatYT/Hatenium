#include "core/PlayTime.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;
using nlohmann::json;

namespace hat {

std::string PlayTimeTracker::Today() {
    time_t t = time(nullptr);
    tm lt{};
    localtime_s(&lt, &t);
    char b[16];
    std::strftime(b, sizeof(b), "%Y-%m-%d", &lt);
    return b;
}

void PlayTimeTracker::Load(const fs::path& dir) {
    file_ = dir / L"playtime.json";
    data_.clear();
    std::ifstream f(file_, std::ios::binary);
    std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    json j = json::parse(txt, nullptr, false);
    if (!j.is_object()) return;
    for (auto d = j.begin(); d != j.end(); ++d) {
        if (!d.value().is_object() || d.key().size() != 10) continue;
        for (auto g = d.value().begin(); g != d.value().end(); ++g)
            if (g.value().is_number()) data_[d.key()][g.key()] = std::max(0.0, g.value().get<double>());
    }
    while (data_.size() > 400) data_.erase(data_.begin());     // keep roughly the last year
}

bool PlayTimeTracker::Save() const {
    json j = json::object();
    for (const auto& d : data_) for (const auto& g : d.second) j[d.first][g.first] = g.second;
    fs::path tmp = file_; tmp += L".tmp";
    { std::ofstream f(tmp, std::ios::binary | std::ios::trunc); if (!f) return false; f << j.dump(1); f.flush(); if (!f) return false; }
    return MoveFileExW(tmp.c_str(), file_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

void PlayTimeTracker::Flush() { if (dirty_ && Save()) dirty_ = false; }

void PlayTimeTracker::Update(float dt, const std::set<std::string>& running, const ToolsSettings& t) {
    running_ = false;
    if (t.trackPlaytime) {
        std::string day = Today();
        for (const auto& exe : t.trackedProcesses) {
            std::string low = exe;
            std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (!low.empty() && running.count(low)) { data_[day][exe] += dt; running_ = true; dirty_ = true; }
        }
    }
    saveTimer_ += dt;
    if (saveTimer_ > 30.f) { saveTimer_ = 0.f; Flush(); }
}

std::vector<PlayTimeTracker::Day> PlayTimeTracker::LastDays(int n) const {
    std::vector<Day> out;
    time_t now = time(nullptr);
    for (int i = n - 1; i >= 0; --i) {
        time_t t = now - static_cast<time_t>(i) * 86400;
        tm lt{};
        localtime_s(&lt, &t);
        char key[16], lab[16];
        std::strftime(key, sizeof(key), "%Y-%m-%d", &lt);
        std::strftime(lab, sizeof(lab), "%m/%d", &lt);
        Day d; d.label = lab;
        auto it = data_.find(key);
        if (it != data_.end()) for (const auto& g : it->second) d.seconds += g.second;
        out.push_back(d);
    }
    return out;
}

std::map<std::string, double> PlayTimeTracker::TotalsByGame(int days) const {
    std::map<std::string, double> out;
    time_t now = time(nullptr);
    for (int i = 0; i < days; ++i) {
        time_t t = now - static_cast<time_t>(i) * 86400;
        tm lt{};
        localtime_s(&lt, &t);
        char key[16];
        std::strftime(key, sizeof(key), "%Y-%m-%d", &lt);
        auto it = data_.find(key);
        if (it != data_.end()) for (const auto& g : it->second) out[g.first] += g.second;
    }
    return out;
}

} // namespace hat
