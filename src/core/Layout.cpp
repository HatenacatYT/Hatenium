#include "core/Layout.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <windows.h>

namespace fs = std::filesystem;
using nlohmann::json;

namespace hat {

void LayoutStore::Load(const fs::path& dir) {
    file_ = dir / L"layout.json";
    data_.clear();
    std::ifstream f(file_, std::ios::binary);
    std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    json j = json::parse(txt, nullptr, false);
    if (!j.is_object() || !j.contains("monitors") || !j["monitors"].is_object()) return;
    auto rd = [](const json& o, const char* k, Pos& p) {
        if (o.contains(k) && o[k].is_array() && o[k].size() == 2 && o[k][0].is_number() && o[k][1].is_number()) {
            p.x = std::min(std::max(o[k][0].get<float>(), 0.f), 100.f);
            p.y = std::min(std::max(o[k][1].get<float>(), 0.f), 100.f);
        }
    };
    for (auto it = j["monitors"].begin(); it != j["monitors"].end(); ++it) {
        if (!it.value().is_object()) continue;
        Entry e;
        rd(it.value(), "keys", e.keys); rd(it.value(), "cps", e.cps); rd(it.value(), "timer", e.timer); rd(it.value(), "watermark", e.watermark);
        data_[it.key()] = e;
    }
}

bool LayoutStore::Save() const {
    json j;
    j["version"] = 1;
    json mons = json::object();
    for (const auto& kv : data_) {
        const Entry& e = kv.second;
        mons[kv.first] = {{"keys", {e.keys.x, e.keys.y}}, {"cps", {e.cps.x, e.cps.y}},
                          {"timer", {e.timer.x, e.timer.y}}, {"watermark", {e.watermark.x, e.watermark.y}}};
    }
    j["monitors"] = mons;
    fs::path tmp = file_; tmp += L".tmp";
    { std::ofstream f(tmp, std::ios::binary | std::ios::trunc); if (!f) return false; f << j.dump(2); f.flush(); if (!f) return false; }
    return MoveFileExW(tmp.c_str(), file_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

void LayoutStore::Apply(const std::string& key, HudSettings& h) const {
    auto it = data_.find(key);
    if (it == data_.end()) return;
    const Entry& e = it->second;
    h.keys.posX = e.keys.x; h.keys.posY = e.keys.y;
    h.cps.posX = e.cps.x; h.cps.posY = e.cps.y;
    h.timer.posX = e.timer.x; h.timer.posY = e.timer.y;
    h.watermark.posX = e.watermark.x; h.watermark.posY = e.watermark.y;
}

void LayoutStore::Store(const std::string& key, const HudSettings& h) {
    Entry e;
    e.keys = {h.keys.posX, h.keys.posY}; e.cps = {h.cps.posX, h.cps.posY};
    e.timer = {h.timer.posX, h.timer.posY}; e.watermark = {h.watermark.posX, h.watermark.posY};
    data_[key] = e;
}

} // namespace hat
