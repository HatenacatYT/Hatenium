#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"
#include "audio/MusicPlayer.h"
#include "audio/YouTubePlayer.h"
#include <cstdio>
#include <string>

namespace hat::gui {

namespace {
std::string TimeStr(float sec) {
    int s = static_cast<int>(sec < 0 ? 0 : sec);
    char b[16];
    std::snprintf(b, sizeof(b), "%02d:%02d", s / 60, s % 60);
    return b;
}

void DrawLocal(Application& app) {
    const Theme& th = ThemeManager::Get().T();
    MusicSettings& m = app.settings().music;
    MusicPlayer& mp = app.music();
    static const char* repeatModes[] = {"Off", "All", "One"};

    int cols = ui::ColumnCountForWidth(380);
    if (!ui::BeginColumns("##music_cols", cols)) return;

    // ------------------------------------------------------------ now playing
    ui::BeginCard("Now Playing");
    if (!mp.Ready()) { ui::NoteText("No audio output device available."); ImGui::Dummy(ImVec2(0, S(2))); }
    bool has = mp.HasTrack();
    std::string title = has && mp.CurrentIndex() >= 0 && mp.CurrentIndex() < static_cast<int>(m.tracks.size())
                            ? MusicPlayer::TitleOf(m.tracks[static_cast<size_t>(mp.CurrentIndex())])
                            : std::string(tr("Nothing playing"));
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextColored(has ? th.text : th.textMuted, "%s", title.c_str());
    ImGui::PopTextWrapPos();
    float dur = mp.Duration(), pos = mp.Position();
    float nf = 0.f;
    if (ui::SeekBar("seek", dur > 0.f ? pos / dur : 0.f, &nf)) mp.SeekFraction(nf);
    std::string t = TimeStr(pos) + " / " + TimeStr(dur);
    ImGui::TextColored(th.textMuted, "%s", t.c_str());
    ImGui::Dummy(ImVec2(0, S(2)));

    float bw = (ImGui::GetContentRegionAvail().x - S(16)) / 3.f;
    if (ui::Button("Prev", ImVec2(bw, 0))) mp.Prev(m);
    ImGui::SameLine(0, S(8));
    if (ui::Button(mp.Playing() ? "Pause" : "Play", ImVec2(bw, 0), true)) mp.TogglePause(m);
    ImGui::SameLine(0, S(8));
    if (ui::Button("Next", ImVec2(bw, 0))) mp.Next(m);
    ImGui::Dummy(ImVec2(0, S(4)));
    float vol = m.volume * 100.f;
    if (ui::SliderFloat("Volume", &vol, 0.f, 100.f, "%.0f%%")) m.volume = vol / 100.f;
    ui::Checkbox("Shuffle", &m.shuffle);
    ui::Combo("Repeat", &m.repeat, repeatModes, 3);
    if (!mp.Error().empty()) { ImGui::Dummy(ImVec2(0, S(2))); ui::NoteText(mp.Error().c_str()); }
    ui::EndCard();

    // ------------------------------------------------------------ playlist
    ui::BeginCard("Playlist");
    float bw2 = (ImGui::GetContentRegionAvail().x - S(16)) / 3.f;
    if (ui::Button("Add Files", ImVec2(bw2, 0), true)) app.AddMusicFiles();
    ImGui::SameLine(0, S(8));
    if (ui::Button("Add Folder", ImVec2(bw2, 0))) app.AddMusicFolder();
    ImGui::SameLine(0, S(8));
    if (ui::Button("Clear", ImVec2(bw2, 0))) mp.Clear(m);
    ui::NoteText("Supports MP3, WAV and FLAC. You can also drag and drop files or folders onto the window.");
    ImGui::Dummy(ImVec2(0, S(2)));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.25f));
    ImGui::BeginChild("##playlist", ImVec2(0, S(250)), ImGuiChildFlags_None, 0);
    ImGui::PopStyleColor();
    if (m.tracks.empty()) ImGui::TextColored(th.textMuted, "%s", tr("The playlist is empty."));
    int remove = -1;
    ImGuiListClipper clip;
    clip.Begin(static_cast<int>(m.tracks.size()));
    while (clip.Step()) {
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
            ImGui::PushID(i);
            char label[512];
            bool cur = (i == mp.CurrentIndex());
            std::snprintf(label, sizeof(label), "%s%02d  %s", cur ? "> " : "   ", i + 1, MusicPlayer::TitleOf(m.tracks[static_cast<size_t>(i)]).c_str());
            if (cur) ImGui::PushStyleColor(ImGuiCol_Text, th.accent);
            if (ImGui::Selectable(label, cur)) mp.Play(m, i);
            if (cur) ImGui::PopStyleColor();
            if (ImGui::BeginPopupContextItem("##ctx")) {
                if (ImGui::MenuItem(tr("Remove"))) remove = i;
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    if (remove >= 0) mp.Remove(m, remove);
    ui::EndCard();

    ui::EndColumns();
}

void DrawYouTube(Application& app) {
    const Theme& th = ThemeManager::Get().T();
    YouTubePlayer& yt = app.youtube();
    MusicSettings& m = app.settings().music;

    if (!YouTubePlayer::Compiled()) {
        ui::BeginCard("YouTube");
        ui::NoteText("This build was compiled without the embedded YouTube player. Build with -DHATENIUM_ENABLE_YOUTUBE=ON (see README) to enable it.");
        ui::EndCard();
        return;
    }

    int cols = ui::ColumnCountForWidth(380);
    if (!ui::BeginColumns("##yt_cols", cols)) return;

    ui::BeginCard("YouTube Player");
    ui::NoteText("Opens YouTube in its own window (a WebView2 browser). Sign in there - with a Premium account playback is ad-free. Hatenium only controls playback.");
    ImGui::Dummy(ImVec2(0, S(4)));
    float bw = (ImGui::GetContentRegionAvail().x - S(8)) * 0.5f;
    if (ui::Button("Open YouTube Music", ImVec2(bw, 0), true)) yt.Open("https://music.youtube.com");
    ImGui::SameLine(0, S(8));
    if (ui::Button("Open YouTube", ImVec2(bw, 0))) yt.Open("https://www.youtube.com");
    if (!yt.Error().empty()) { ImGui::Dummy(ImVec2(0, S(2))); ui::NoteText(yt.Error().c_str()); }

    ImGui::Dummy(ImVec2(0, S(6)));
    static char query[256] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    bool enter = ImGui::InputTextWithHint("##ytq", tr("Search YouTube..."), query, sizeof(query), ImGuiInputTextFlags_EnterReturnsTrue);
    if (ui::Button("Search Music", ImVec2(bw, 0)) || enter) yt.Search(query, true);
    ImGui::SameLine(0, S(8));
    if (ui::Button("Search Videos", ImVec2(bw, 0))) yt.Search(query, false);
    ui::EndCard();

    ui::BeginCard("Now Playing");
    const auto& info = yt.GetInfo();
    if (!yt.IsOpen()) ui::NoteText("The player window is closed.");
    else {
        ImGui::PushTextWrapPos(0.f);
        ImGui::TextColored(info.valid ? th.text : th.textMuted, "%s", info.valid && !info.title.empty() ? info.title.c_str() : tr("Nothing playing"));
        if (!info.artist.empty()) ImGui::TextColored(th.textMuted, "%s", info.artist.c_str());
        ImGui::PopTextWrapPos();
        char t[48]; int p = static_cast<int>(info.pos), d = static_cast<int>(info.dur);
        std::snprintf(t, sizeof(t), "%02d:%02d / %02d:%02d", p / 60, p % 60, d / 60, d % 60);
        ImGui::TextColored(th.textMuted, "%s", t);
    }
    ImGui::Dummy(ImVec2(0, S(4)));
    float b3 = (ImGui::GetContentRegionAvail().x - S(16)) / 3.f;
    if (ui::Button("Prev", ImVec2(b3, 0))) yt.Prev();
    ImGui::SameLine(0, S(8));
    if (ui::Button(yt.GetInfo().playing ? "Pause" : "Play", ImVec2(b3, 0), true)) yt.PlayPause();
    ImGui::SameLine(0, S(8));
    if (ui::Button("Next", ImVec2(b3, 0))) yt.Next();
    ImGui::Dummy(ImVec2(0, S(4)));
    float vol = m.volume * 100.f;
    if (ui::SliderFloat("Volume", &vol, 0.f, 100.f, "%.0f%%")) { m.volume = vol / 100.f; yt.SetVolume(m.volume); }
    if (yt.IsOpen() && ui::Button("Close Player", ImVec2(ImGui::GetContentRegionAvail().x, 0))) yt.Close();
    ui::EndCard();

    ui::EndColumns();
}
} // namespace

void DrawMusicPage(Application& app) {
    MusicSettings& m = app.settings().music;
    float bw = std::min(ImGui::GetContentRegionAvail().x * 0.5f - S(4), S(150));
    if (ui::Button("Local", ImVec2(bw, 0), m.source == 0)) m.source = 0;
    ImGui::SameLine(0, S(8));
    if (ui::Button("YouTube", ImVec2(bw, 0), m.source == 1)) m.source = 1;
    ImGui::Dummy(ImVec2(0, S(4)));
    if (m.source == 0) DrawLocal(app); else DrawYouTube(app);
}

} // namespace hat::gui
