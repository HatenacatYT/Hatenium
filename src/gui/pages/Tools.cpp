#include "gui/Screens.h"
#include "gui/Components.h"
#include "gui/ThemeManager.h"
#include "core/Localization.h"
#include "core/ToolsManager.h"
#include <algorithm>
#include <cstdio>
#include <string>

namespace hat::gui {

namespace {

void DrawPlaytimeChart(const std::vector<PlayTimeTracker::Day>& days) {
    const Theme& th = ThemeManager::Get().T();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = ImGui::GetContentRegionAvail().x, h = S(130);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h));
    double mx = 60.0;
    for (const auto& d : days) mx = std::max(mx, d.seconds);
    float labelH = S(18), top = S(16);
    float chartH = h - labelH - top;
    int n = static_cast<int>(days.size());
    float slot = w / static_cast<float>(n), bw = slot * 0.55f;
    dl->AddLine(ImVec2(p.x, p.y + top + chartH), ImVec2(p.x + w, p.y + top + chartH), C32(th.border));
    for (int i = 0; i < n; ++i) {
        float x = p.x + slot * i + (slot - bw) * 0.5f;
        float bh = static_cast<float>(days[static_cast<size_t>(i)].seconds / mx) * chartH;
        bool today = (i == n - 1);
        dl->AddRectFilled(ImVec2(x, p.y + top + chartH - bh), ImVec2(x + bw, p.y + top + chartH), C32(today ? th.accent : WithAlpha(th.accent, 0.55f)), std::min(S(4), th.buttonRounding));
        const char* lab = days[static_cast<size_t>(i)].label.c_str();
        ImVec2 ts = ImGui::CalcTextSize(lab);
        dl->AddText(ImVec2(x + (bw - ts.x) * 0.5f, p.y + h - labelH + S(2)), C32(th.textMuted), lab);
        double sec = days[static_cast<size_t>(i)].seconds;
        if (sec >= 60.0) {
            char v[24];
            if (sec >= 3600.0) std::snprintf(v, sizeof(v), "%.1fh", sec / 3600.0); else std::snprintf(v, sizeof(v), "%dm", static_cast<int>(sec / 60.0));
            ImVec2 vs = ImGui::CalcTextSize(v);
            dl->AddText(ImVec2(x + (bw - vs.x) * 0.5f, p.y + top + chartH - bh - vs.y - S(2)), C32(th.textDim), v);
        }
    }
}

} // namespace

void DrawToolsPage(Application& app) {
    const Theme& th = ThemeManager::Get().T();
    Settings& s = app.settings();
    ToolsManager& tm = app.tools();

    int cols = ui::ColumnCountForWidth(380);
    if (!ui::BeginColumns("##tools_cols", cols)) return;

    // ------------------------------------------------------------ timer
    ui::BeginCard("Timer");
    {
        ImFont* f = ImGui::GetFont();
        std::string t = ToolsManager::Format(tm.TimerRunning() || tm.TimerRemaining() > 0 ? tm.TimerRemaining() + (tm.TimerRunning() ? 0.999 : 0.0) : 0.0);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float fs = f->FontSize * 2.3f;
        ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0.f, t.c_str());
        float w = ImGui::GetContentRegionAvail().x;
        ImGui::Dummy(ImVec2(w, ts.y + S(4)));
        ImGui::GetWindowDrawList()->AddText(f, fs, ImVec2(p.x + (w - ts.x) * 0.5f, p.y), C32(tm.TimerRunning() ? th.accent : th.text), t.c_str());
    }
    if (!tm.TimerRunning()) {
        int mins = s.tools.timerMinutes, secs = s.tools.timerSeconds;
        bool ch = ui::SliderInt("Minutes", &mins, 0, 180, "%d min");
        ch |= ui::SliderInt("Seconds", &secs, 0, 59, "%d s");
        if (ch) { s.tools.timerMinutes = mins; s.tools.timerSeconds = secs; tm.TimerSet(mins * 60 + secs); }
        else if (tm.TimerTotal() == 0.0 && (mins || secs)) tm.TimerSet(mins * 60 + secs);
    }
    float bw = (ImGui::GetContentRegionAvail().x - S(8)) * 0.5f;
    if (ui::Button(tm.TimerRunning() ? "Pause" : "Start", ImVec2(bw, 0), true)) { if (!tm.TimerRunning() && tm.TimerTotal() == 0.0) tm.TimerSet(s.tools.timerMinutes * 60 + s.tools.timerSeconds); tm.TimerStartPause(); }
    ImGui::SameLine(0, S(8));
    if (ui::Button("Reset", ImVec2(bw, 0))) tm.TimerReset();
    ui::EndCard();

    ui::BeginCard("Stopwatch");
    {
        ImFont* f = ImGui::GetFont();
        std::string t = ToolsManager::Format(tm.StopwatchElapsed());
        ImVec2 p = ImGui::GetCursorScreenPos();
        float fs = f->FontSize * 2.3f;
        ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0.f, t.c_str());
        float w = ImGui::GetContentRegionAvail().x;
        ImGui::Dummy(ImVec2(w, ts.y + S(4)));
        ImGui::GetWindowDrawList()->AddText(f, fs, ImVec2(p.x + (w - ts.x) * 0.5f, p.y), C32(tm.StopwatchRunning() ? th.accent : th.text), t.c_str());
    }
    if (ui::Button(tm.StopwatchRunning() ? "Stop" : "Start", ImVec2(bw, 0), true)) tm.StopwatchStartStop();
    ImGui::SameLine(0, S(8));
    if (ui::Button("Reset", ImVec2(bw, 0))) tm.StopwatchReset();
    ui::NoteText("Turn on \"Timer HUD\" in the HUD tab to show the running time on your screen.");
    ui::EndCard();

    // ------------------------------------------------------------ playtime
    ui::BeginCard("Play Time");
    ui::Checkbox("Track Play Time", &s.tools.trackPlaytime);
    ui::NoteText("Measures how long the listed programs are running (a local playtime.json - nothing is sent anywhere).");
    ImGui::Dummy(ImVec2(0, S(2)));
    DrawPlaytimeChart(app.playtime().LastDays(7));
    {
        auto totals = app.playtime().TotalsByGame(7);
        for (const auto& kv : totals) {
            char b[160];
            std::snprintf(b, sizeof(b), "%s  -  %.1f h", kv.first.c_str(), kv.second / 3600.0);
            ImGui::TextColored(th.textDim, "%s", b);
        }
        if (app.playtime().Running()) ImGui::TextColored(ImVec4(0.45f, 0.95f, 0.65f, 1), "%s", tr("Recording now"));
    }
    ImGui::Dummy(ImVec2(0, S(4)));
    ImGui::TextColored(th.textMuted, "%s", tr("Tracked programs"));
    int removeIdx = -1;
    for (size_t i = 0; i < s.tools.trackedProcesses.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::TextUnformatted(s.tools.trackedProcesses[i].c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - S(24) + ImGui::GetCursorPosX() - S(4));
        if (ImGui::SmallButton("x")) removeIdx = static_cast<int>(i);
        ImGui::PopID();
    }
    if (removeIdx >= 0) s.tools.trackedProcesses.erase(s.tools.trackedProcesses.begin() + removeIdx);
    static char exe[96] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    bool enter = ImGui::InputTextWithHint("##addexe", tr("Add program, e.g. game.exe"), exe, sizeof(exe), ImGuiInputTextFlags_EnterReturnsTrue);
    if ((ui::Button("Add Program", ImVec2(ImGui::GetContentRegionAvail().x, 0)) || enter) && exe[0] && s.tools.trackedProcesses.size() < 20) {
        s.tools.trackedProcesses.push_back(exe); exe[0] = 0;
    }
    ui::EndCard();

    // ------------------------------------------------------------ server region
    ui::BeginCard("Server Region");
    static ServerRegion::Result res;
    static float auto_ = 100.f;
    auto_ += ImGui::GetIO().DeltaTime;
    bool run = ui::Button("Check Now", ImVec2(ImGui::GetContentRegionAvail().x, 0), true);
    bool busy = app.region().Busy();
    if (run || auto_ > (busy ? 0.7f : 20.f)) { auto_ = 0.f; res = app.region().Refresh(s.tools.onlineIpLookup); }
    if (res.found) {
        ImGui::TextColored(th.textMuted, "%s", tr("Server IP"));
        ImGui::TextUnformatted(res.ip.c_str());
        ImGui::TextColored(th.textMuted, "%s", tr("Estimated region"));
        const char* shown = !res.region.empty() ? res.region.c_str() : (busy ? tr("Looking up the location...") : tr("Unknown"));
        ImGui::TextColored(res.region.empty() ? th.textDim : th.accent, "%s", shown);
    }
    if (!res.note.empty() && !(busy && res.region.empty())) { ImGui::Dummy(ImVec2(0, S(2))); ui::NoteText(res.note.c_str()); }
    ImGui::Dummy(ImVec2(0, S(4)));
    ui::Checkbox("Online IP Lookup", &s.tools.onlineIpLookup);
    ui::NoteText("Reads the Roblox client log (a plain file read) and finds the location of the server IP. With Online IP Lookup on, only that server IP address is sent to ipwho.is (nothing else, nothing to the developer); a local geoip.csv is used first when present. This is only an estimate of the connection region, not an exact location or ping.");
    ui::EndCard();

    ui::EndColumns();
}

} // namespace hat::gui
