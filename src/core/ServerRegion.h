#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace hat {

// Estimates the region of the Roblox server you are connected to: reads the newest client log under
// %LOCALAPPDATA%\Roblox\logs (a plain file read), extracts the server IP and looks it up in a LOCAL GeoIP CSV.
// It is only an estimate - the real ping is not available.
class ServerRegion {
public:
    struct Result { bool found = false; std::string ip, region, note; };
    void SetDataDir(const std::filesystem::path& configDir, const std::filesystem::path& exeDir);
    // allowOnline: when the local geoip.csv has no answer, look the server IP up with a free online service
    // (https://ipwho.is - only the server's IP address is requested). The lookup runs in the background.
    Result Refresh(bool allowOnline);
    bool Busy() const;
    struct GeoInfo { bool ok = false; std::string text; double when = 0.0; };
    static bool ParseIpwho(const std::string& json, GeoInfo& out);
    bool HasDatabase() const { return !db_.empty(); }
    const std::filesystem::path& DatabasePath() const { return dbPath_; }

private:
    struct Range { uint32_t start, end; std::string region; };
    void LoadDatabase();
    std::string Lookup(uint32_t ip) const;
    std::vector<Range> db_;
    std::filesystem::path dbPath_, configDir_, exeDir_;
    bool dbTried_ = false;
    struct Shared { std::mutex m; std::map<std::string, GeoInfo> cache; std::set<std::string> inflight; std::map<std::string, double> started; };
    std::shared_ptr<Shared> shared_ = std::make_shared<Shared>();
};

} // namespace hat
