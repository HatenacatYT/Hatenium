#include "core/ServerRegion.h"
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <regex>
#include <thread>
#include <winhttp.h>
#include <nlohmann/json.hpp>
#include <sstream>

namespace fs = std::filesystem;

namespace hat {

namespace {
bool ParseIpv4(const std::string& s, uint32_t& out) {
    unsigned a, b, c, d;
    char tail;
    if (std::sscanf(s.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4 || a > 255 || b > 255 || c > 255 || d > 255) return false;
    out = (a << 24) | (b << 16) | (c << 8) | d;
    return true;
}
double NowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

bool IsPlainIpv4(const std::string& s) {
    if (s.size() < 7 || s.size() > 15) return false;
    for (char c : s) if (!(c == '.' || (c >= '0' && c <= '9'))) return false;
    return true;
}

// Small HTTPS GET with WinHTTP (system proxy settings are honoured). Returns an empty string on any failure.
std::string HttpsGet(const wchar_t* host, const std::wstring& path, DWORD timeoutMs) {
    std::string body;
    HINTERNET ses = WinHttpOpen(L"Hatenium", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) return body;
    WinHttpSetTimeouts(ses, static_cast<int>(timeoutMs), static_cast<int>(timeoutMs), static_cast<int>(timeoutMs), static_cast<int>(timeoutMs));
    HINTERNET con = WinHttpConnect(ses, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET req = con ? WinHttpOpenRequest(con, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
    if (req && WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(req, nullptr)) {
        DWORD status = 0, sz = sizeof(status);
        WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);
        if (status == 200) {
            for (;;) {
                DWORD avail = 0;
                if (!WinHttpQueryDataAvailable(req, &avail) || avail == 0) break;
                std::string chunk(avail, '\0');
                DWORD got = 0;
                if (!WinHttpReadData(req, &chunk[0], avail, &got) || got == 0) break;
                body.append(chunk.data(), got);
                if (body.size() > 64 * 1024) break;
            }
        }
    }
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
    return body;
}

std::string Strip(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '"'), s.end());
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ')) s.pop_back();
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    return s;
}
}

void ServerRegion::SetDataDir(const fs::path& configDir, const fs::path& exeDir) { configDir_ = configDir; exeDir_ = exeDir; dbTried_ = false; db_.clear(); }

// CSV format (DB-IP / IP2Location-LITE style): start_ip,end_ip,<region columns...>   (IPv6 rows are skipped)
void ServerRegion::LoadDatabase() {
    dbTried_ = true;
    for (const fs::path& p : {configDir_ / L"geoip.csv", exeDir_ / L"assets" / L"geoip.csv", exeDir_ / L"geoip.csv"}) {
        std::error_code ec;
        if (!fs::exists(p, ec)) continue;
        std::ifstream f(p, std::ios::binary);
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#' || line.find(':') != std::string::npos) continue;
            std::vector<std::string> cols;
            std::stringstream ss(line);
            std::string c;
            while (std::getline(ss, c, ',')) cols.push_back(Strip(c));
            uint32_t a, b;
            if (cols.size() < 3 || !ParseIpv4(cols[0], a) || !ParseIpv4(cols[1], b)) continue;
            std::string region;
            for (size_t i = 2; i < cols.size(); ++i) if (!cols[i].empty()) { if (!region.empty()) region += ", "; region += cols[i]; }
            db_.push_back({a, b, region});
        }
        std::sort(db_.begin(), db_.end(), [](const Range& x, const Range& y) { return x.start < y.start; });
        dbPath_ = p;
        if (!db_.empty()) return;
    }
}

std::string ServerRegion::Lookup(uint32_t ip) const {
    auto it = std::upper_bound(db_.begin(), db_.end(), ip, [](uint32_t v, const Range& r) { return v < r.start; });
    if (it == db_.begin()) return {};
    --it;
    return (ip >= it->start && ip <= it->end) ? it->region : std::string();
}

bool ServerRegion::ParseIpwho(const std::string& json, GeoInfo& out) {
    auto j = nlohmann::json::parse(json, nullptr, false);
    if (!j.is_object() || !j.value("success", false)) return false;
    std::string city = j.value("city", std::string()), region = j.value("region", std::string()), country = j.value("country", std::string());
    std::string text;
    for (const std::string* p : {&city, &region, &country}) {
        if (p->empty() || text.find(*p) != std::string::npos) continue;
        if (!text.empty()) text += ", ";
        text += *p;
    }
    std::string org;
    if (j.contains("connection") && j["connection"].is_object()) org = j["connection"].value("org", std::string());
    if (!org.empty()) text += (text.empty() ? "" : "  ") + std::string("(") + org + ")";
    if (text.empty()) return false;
    out.ok = true; out.text = text;
    return true;
}

bool ServerRegion::Busy() const {
    std::lock_guard<std::mutex> lk(shared_->m);
    return !shared_->inflight.empty();
}

ServerRegion::Result ServerRegion::Refresh(bool allowOnline) {
    Result r;
    if (!dbTried_) LoadDatabase();

    wchar_t* base = _wgetenv(L"LOCALAPPDATA");
    if (!base) { r.note = "LOCALAPPDATA is not set"; return r; }
    fs::path dir = fs::path(base) / L"Roblox" / L"logs";
    std::error_code ec;
    if (!fs::exists(dir, ec)) { r.note = "No Roblox log folder found (is Roblox installed?)"; return r; }

    fs::path newest; fs::file_time_type best{};
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!e.is_regular_file(ec) || e.path().extension() != L".log") continue;
        auto t = e.last_write_time(ec);
        if (newest.empty() || t > best) { newest = e.path(); best = t; }
    }
    if (newest.empty()) { r.note = "No Roblox log file found"; return r; }

    // read only the tail of the newest log
    std::ifstream f(newest, std::ios::binary | std::ios::ate);
    if (!f) { r.note = "Could not read the Roblox log"; return r; }
    std::streamoff size = f.tellg();
    std::streamoff take = std::min<std::streamoff>(size, 1024 * 1024);
    f.seekg(size - take);
    std::string tail(static_cast<size_t>(take), '\0');
    f.read(&tail[0], take);

    static const std::regex re(R"((?:UDMUX Address\s*=\s*|Connecting to UDP server\s*|UDP server\s*)(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
    std::string last;
    for (auto it = std::sregex_iterator(tail.begin(), tail.end(), re); it != std::sregex_iterator(); ++it) last = (*it)[1].str();
    if (last.empty()) { r.note = "No server connection found in the latest log yet"; return r; }

    r.found = true; r.ip = last;
    uint32_t ip = 0;
    if (!db_.empty() && ParseIpv4(last, ip)) r.region = Lookup(ip);
    if (!r.region.empty()) { r.note = "Estimated from the IP address - not an exact location or ping"; return r; }

    if (allowOnline && IsPlainIpv4(last)) {
        auto sh = shared_;
        bool start = false;
        {
            std::lock_guard<std::mutex> lk(sh->m);
            // watchdog: a lookup that hangs (no network, blocked DNS) is reported as failed after 12 s
            if (sh->inflight.count(last) && NowSec() - sh->started[last] > 12.0) {
                sh->inflight.erase(last);
                GeoInfo fail; fail.when = NowSec();
                sh->cache[last] = fail;
            }
            auto it = sh->cache.find(last);
            if (it != sh->cache.end() && !it->second.ok && NowSec() - it->second.when > 30.0) { sh->cache.erase(it); it = sh->cache.end(); }   // retry failures after a while
            if (it != sh->cache.end()) {
                if (it->second.ok) { r.region = it->second.text; r.note = "Estimated from the IP address (online lookup) - not an exact location or ping"; }
                else r.note = "Online lookup failed - check your internet connection";
                return r;
            }
            if (!sh->inflight.count(last)) { sh->inflight.insert(last); sh->started[last] = NowSec(); start = true; }
        }
        if (start) {
            std::string ip = last;
            std::thread([sh, ip]() {
                GeoInfo gi;
                std::string body = HttpsGet(L"ipwho.is", L"/" + std::wstring(ip.begin(), ip.end()), 5000);
                if (!body.empty()) ParseIpwho(body, gi);
                gi.when = NowSec();
                std::lock_guard<std::mutex> lk(sh->m);
                if (gi.ok || !sh->cache.count(ip)) sh->cache[ip] = gi;   // a late failure must not hide an earlier answer
                sh->inflight.erase(ip);
            }).detach();
        }
        r.note = "Looking up the location...";
        return r;
    }
    r.note = db_.empty() ? "Online lookup is off and no geoip.csv was found" : "This IP is not in the GeoIP database";
    return r;
}

} // namespace hat
