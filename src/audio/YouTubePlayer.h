#pragma once
#include <string>

namespace hat {

// Optional YouTube / YouTube Music player: a WebView2 window (the real youtube.com in a browser control) that Hatenium
// controls with small JavaScript snippets. Compiled only with -DHATENIUM_ENABLE_YOUTUBE=ON; otherwise every call is a no-op.
// Signing in to a YouTube Premium account inside that window gives the normal ad-free playback.
class YouTubePlayer {
public:
    struct Info { std::string title, artist; bool playing = false; float pos = 0.f, dur = 0.f; bool valid = false; };

    YouTubePlayer();
    ~YouTubePlayer();
    static bool Compiled();

    bool IsOpen() const { return web_ != nullptr; }
    bool Open(const std::string& url);
    void Close();
    void Search(const std::string& query, bool music);
    void PlayPause();
    void Next();
    void Prev();
    void SetVolume(float v);
    void Update();                       // call every frame: detects that the user closed the window
    const Info& GetInfo() const { return info_; }
    const std::string& Error() const { return error_; }
    void OnNowPlaying(const std::string& json);   // called from the JS bridge

private:
    void Eval(const char* js);
    void* web_ = nullptr;
    Info info_;
    std::string error_;
};

} // namespace hat
