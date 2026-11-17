#pragma once
#include <memory>
#include <string>
#include <vector>
#include "core/Settings.h"

namespace hat {

// Local-file music player (WAV / MP3 / FLAC through miniaudio). The playlist lives in MusicSettings so it is saved with the config.
class MusicPlayer {
public:
    MusicPlayer();
    ~MusicPlayer();

    bool Init();                                   // opens the default audio device; false = no device
    void Shutdown();
    bool Ready() const;
    const std::string& Error() const { return error_; }

    void Update(MusicSettings& m);                 // call every frame: volume, end-of-track handling
    void AddPaths(MusicSettings& m, const std::vector<std::wstring>& paths);   // files and/or folders
    void Clear(MusicSettings& m);
    void Remove(MusicSettings& m, int index);
    bool Play(MusicSettings& m, int index);
    void TogglePause(MusicSettings& m);
    void Stop();
    void Next(MusicSettings& m);
    void Prev(MusicSettings& m);
    void SeekFraction(float f);

    bool HasTrack() const;
    bool Playing() const;
    float Position() const;                        // seconds
    float Duration() const;                        // seconds (0 if unknown)
    int  CurrentIndex() const { return loadedIndex_; }
    static std::string TitleOf(const std::string& utf8Path);
    static bool IsSupported(const std::wstring& path);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string error_;
    int loadedIndex_ = -1;
    bool paused_ = false;
    unsigned rng_ = 2463534242u;
};

} // namespace hat
