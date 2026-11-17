#include "audio/YouTubePlayer.h"

#ifdef HATENIUM_YOUTUBE
#define WEBVIEW_STATIC
#define WEBVIEW_HEADER            // declarations only - the implementation lives in WebviewImpl.cpp
#include "webview.h"
#include <windows.h>
#include <nlohmann/json.hpp>
#include <cstdio>

namespace hat {

namespace {
const char* kPollJs = R"JS(
(function(){
  if (window.__hatPoll) return;
  window.__hatPoll = setInterval(function(){
    try {
      var v = document.querySelector('video');
      var tEl = document.querySelector('ytmusic-player-bar .title, h1.ytd-watch-metadata yt-formatted-string, h1.title');
      var aEl = document.querySelector('ytmusic-player-bar .byline, ytd-channel-name a');
      var t = (tEl && tEl.textContent) || document.title;
      var a = (aEl && aEl.textContent) || '';
      window.hatenium_np(JSON.stringify({t: t.trim(), a: a.trim(), p: v ? !v.paused : false,
        c: v ? v.currentTime : 0, d: (v && isFinite(v.duration)) ? v.duration : 0}));
    } catch (e) {}
  }, 1000);
})();
)JS";

std::string UrlEncode(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string o;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o += static_cast<char>(c);
        else { o += '%'; o += hex[c >> 4]; o += hex[c & 15]; }
    }
    return o;
}

void BridgeCb(const char* id, const char* req, void* arg) {
    auto* self = static_cast<hat::YouTubePlayer*>(arg);
    auto j = nlohmann::json::parse(req, nullptr, false);
    if (j.is_array() && !j.empty() && j[0].is_string()) self->OnNowPlaying(j[0].get<std::string>());
    (void)id;
}
} // namespace

YouTubePlayer::YouTubePlayer() {}
YouTubePlayer::~YouTubePlayer() { Close(); }
bool YouTubePlayer::Compiled() { return true; }

bool YouTubePlayer::Open(const std::string& url) {
    if (!web_) {
        webview_t w = webview_create(0, nullptr);
        if (!w) { error_ = "WebView2 runtime not found (install the Microsoft Edge WebView2 Runtime)"; return false; }
        webview_set_title(w, "Hatenium - YouTube");
        webview_set_size(w, 1100, 720, WEBVIEW_HINT_NONE);
        webview_bind(w, "hatenium_np", BridgeCb, this);
        webview_init(w, kPollJs);
        web_ = w;
        error_.clear();
    }
    webview_navigate(static_cast<webview_t>(web_), url.c_str());
    return true;
}

void YouTubePlayer::Close() {
    if (!web_) return;
    webview_destroy(static_cast<webview_t>(web_));
    web_ = nullptr;
    info_ = Info();
}

void YouTubePlayer::Update() {
    if (!web_) return;
    HWND h = static_cast<HWND>(webview_get_window(static_cast<webview_t>(web_)));
    if (!h || !IsWindow(h)) Close();
}

void YouTubePlayer::Eval(const char* js) { if (web_) webview_eval(static_cast<webview_t>(web_), js); }

void YouTubePlayer::Search(const std::string& q, bool music) {
    if (q.empty()) return;
    std::string url = music ? "https://music.youtube.com/search?q=" + UrlEncode(q)
                            : "https://www.youtube.com/results?search_query=" + UrlEncode(q);
    Open(url);
}

void YouTubePlayer::PlayPause() { Eval("(function(){var v=document.querySelector('video');if(v){if(v.paused)v.play();else v.pause();}})()"); }
void YouTubePlayer::Next() { Eval("(function(){var b=document.querySelector('.next-button, .ytp-next-button');if(b)b.click();})()"); }
void YouTubePlayer::Prev() { Eval("(function(){var b=document.querySelector('.previous-button, .ytp-prev-button');if(b)b.click();})()"); }

void YouTubePlayer::SetVolume(float v) {
    char js[160];
    std::snprintf(js, sizeof(js), "(function(){var v=document.querySelector('video');if(v)v.volume=%.3f;})()", v < 0 ? 0.f : (v > 1 ? 1.f : v));
    Eval(js);
}

void YouTubePlayer::OnNowPlaying(const std::string& json) {
    auto j = nlohmann::json::parse(json, nullptr, false);
    if (!j.is_object()) return;
    info_.title = j.value("t", std::string());
    info_.artist = j.value("a", std::string());
    info_.playing = j.value("p", false);
    info_.pos = j.value("c", 0.f);
    info_.dur = j.value("d", 0.f);
    info_.valid = true;
}

} // namespace hat

#else  // ---------------------------------------------------------------- built without WebView2

namespace hat {
YouTubePlayer::YouTubePlayer() {}
YouTubePlayer::~YouTubePlayer() {}
bool YouTubePlayer::Compiled() { return false; }
bool YouTubePlayer::Open(const std::string&) { error_ = "Built without YouTube support"; return false; }
void YouTubePlayer::Close() {}
void YouTubePlayer::Search(const std::string&, bool) {}
void YouTubePlayer::PlayPause() {}
void YouTubePlayer::Next() {}
void YouTubePlayer::Prev() {}
void YouTubePlayer::SetVolume(float) {}
void YouTubePlayer::Update() {}
void YouTubePlayer::OnNowPlaying(const std::string&) {}
void YouTubePlayer::Eval(const char*) {}
} // namespace hat

#endif
