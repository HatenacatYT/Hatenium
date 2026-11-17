#include "audio/MusicPlayer.h"
#include "audio/miniaudio_config.h"
#include "core/Utf.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace hat {

struct MusicPlayer::Impl {
    ma_engine engine{};
    ma_sound sound{};
    bool engineOk = false;
    bool soundOk = false;
    ma_uint32 sampleRate = 44100;
};

MusicPlayer::MusicPlayer() : impl_(new Impl) {}
MusicPlayer::~MusicPlayer() { Shutdown(); }

bool MusicPlayer::Init() {
    ma_engine_config cfg = ma_engine_config_init();
    ma_result r = ma_engine_init(&cfg, &impl_->engine);
    impl_->engineOk = (r == MA_SUCCESS);
    if (!impl_->engineOk) error_ = "No audio output device available";
    return impl_->engineOk;
}

void MusicPlayer::Shutdown() {
    if (!impl_) return;
    Stop();
    if (impl_->engineOk) { ma_engine_uninit(&impl_->engine); impl_->engineOk = false; }
}

bool MusicPlayer::Ready() const { return impl_ && impl_->engineOk; }
bool MusicPlayer::HasTrack() const { return impl_->soundOk; }
bool MusicPlayer::Playing() const { return impl_->soundOk && !paused_ && ma_sound_is_playing(&impl_->sound); }

float MusicPlayer::Position() const {
    if (!impl_->soundOk) return 0.f;
    ma_uint64 f = 0;
    ma_sound_get_cursor_in_pcm_frames(&impl_->sound, &f);
    return static_cast<float>(f) / static_cast<float>(impl_->sampleRate);
}

float MusicPlayer::Duration() const {
    if (!impl_->soundOk) return 0.f;
    ma_uint64 n = 0;
    if (ma_sound_get_length_in_pcm_frames(&impl_->sound, &n) != MA_SUCCESS) return 0.f;
    return static_cast<float>(n) / static_cast<float>(impl_->sampleRate);
}

std::string MusicPlayer::TitleOf(const std::string& utf8Path) {
    fs::path p(WideFromUtf8(utf8Path));
    return Utf8FromWide(p.stem().wstring());
}

bool MusicPlayer::IsSupported(const std::wstring& path) {
    std::wstring ext = fs::path(path).extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return ext == L".mp3" || ext == L".wav" || ext == L".flac";
}

void MusicPlayer::AddPaths(MusicSettings& m, const std::vector<std::wstring>& paths) {
    std::vector<std::wstring> found;
    std::error_code ec;
    for (const auto& p : paths) {
        if (fs::is_directory(p, ec)) {
            std::vector<std::wstring> sub;
            for (fs::recursive_directory_iterator it(p, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
                if (ec) break;
                if (it->is_regular_file(ec) && IsSupported(it->path().wstring())) sub.push_back(it->path().wstring());
                if (sub.size() > 3000) break;
            }
            std::sort(sub.begin(), sub.end());
            found.insert(found.end(), sub.begin(), sub.end());
        } else if (IsSupported(p)) {
            found.push_back(p);
        }
    }
    for (const auto& w : found) {
        if (m.tracks.size() >= 5000) break;
        std::string u = Utf8FromWide(w);
        if (std::find(m.tracks.begin(), m.tracks.end(), u) == m.tracks.end()) m.tracks.push_back(u);
    }
}

void MusicPlayer::Clear(MusicSettings& m) { Stop(); m.tracks.clear(); m.current = -1; }

void MusicPlayer::Remove(MusicSettings& m, int index) {
    if (index < 0 || index >= static_cast<int>(m.tracks.size())) return;
    bool wasLoaded = (index == loadedIndex_);
    m.tracks.erase(m.tracks.begin() + index);
    if (wasLoaded) Stop();
    else if (loadedIndex_ > index) --loadedIndex_;
    if (m.current == index) m.current = -1;
    else if (m.current > index) --m.current;
}

bool MusicPlayer::Play(MusicSettings& m, int index) {
    if (!Ready() || index < 0 || index >= static_cast<int>(m.tracks.size())) return false;
    Stop();
    std::wstring w = WideFromUtf8(m.tracks[static_cast<size_t>(index)]);
    ma_result r = ma_sound_init_from_file_w(&impl_->engine, w.c_str(), MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION,
                                            nullptr, nullptr, &impl_->sound);
    if (r != MA_SUCCESS) { error_ = "Could not open: " + TitleOf(m.tracks[static_cast<size_t>(index)]); return false; }
    impl_->soundOk = true;
    ma_format fmt; ma_uint32 ch = 0, sr = 0;
    if (ma_sound_get_data_format(&impl_->sound, &fmt, &ch, &sr, nullptr, 0) == MA_SUCCESS && sr > 0) impl_->sampleRate = sr;
    ma_sound_set_volume(&impl_->sound, m.volume);
    ma_sound_start(&impl_->sound);
    loadedIndex_ = index; m.current = index; paused_ = false; error_.clear();
    return true;
}

void MusicPlayer::Stop() {
    if (impl_->soundOk) { ma_sound_uninit(&impl_->sound); impl_->soundOk = false; }
    loadedIndex_ = -1; paused_ = false;
}

void MusicPlayer::TogglePause(MusicSettings& m) {
    if (!impl_->soundOk) {
        int i = (m.current >= 0 && m.current < static_cast<int>(m.tracks.size())) ? m.current : 0;
        Play(m, i);
        return;
    }
    if (paused_) { ma_sound_start(&impl_->sound); paused_ = false; }
    else         { ma_sound_stop(&impl_->sound);  paused_ = true; }
}

void MusicPlayer::Next(MusicSettings& m) {
    int n = static_cast<int>(m.tracks.size());
    if (n == 0) return;
    int cur = loadedIndex_ >= 0 ? loadedIndex_ : m.current;
    int next;
    if (m.shuffle && n > 1) {
        do { rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5; next = static_cast<int>(rng_ % static_cast<unsigned>(n)); } while (next == cur);
    } else next = (cur + 1) % n;
    Play(m, next);
}

void MusicPlayer::Prev(MusicSettings& m) {
    int n = static_cast<int>(m.tracks.size());
    if (n == 0) return;
    if (Position() > 3.f && impl_->soundOk) { SeekFraction(0.f); return; }
    int cur = loadedIndex_ >= 0 ? loadedIndex_ : m.current;
    Play(m, (cur <= 0 ? n : cur) - 1);
}

void MusicPlayer::SeekFraction(float f) {
    if (!impl_->soundOk) return;
    float d = Duration();
    if (d <= 0.f) return;
    f = std::min(std::max(f, 0.f), 1.f);
    ma_sound_seek_to_pcm_frame(&impl_->sound, static_cast<ma_uint64>(f * d * static_cast<float>(impl_->sampleRate)));
}

void MusicPlayer::Update(MusicSettings& m) {
    if (!impl_->soundOk) return;
    ma_sound_set_volume(&impl_->sound, m.volume);
    if (!paused_ && ma_sound_at_end(&impl_->sound)) {
        int n = static_cast<int>(m.tracks.size());
        if (m.repeat == 2) { int i = loadedIndex_; Play(m, i); }
        else if (m.repeat == 1 || m.shuffle || loadedIndex_ + 1 < n) Next(m);
        else Stop();
    }
}

} // namespace hat
