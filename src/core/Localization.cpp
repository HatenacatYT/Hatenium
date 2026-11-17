#include "core/Localization.h"
#include <string>
#include <unordered_map>

namespace hat {

namespace {
int g_lang = 0;

const std::unordered_map<std::string, const char*>& Table() {
    static const std::unordered_map<std::string, const char*> t = {
        // tabs / pages
        {"Combat", "戦闘"}, {"Visuals", "ビジュアル"}, {"Movement", "移動"}, {"Misc", "その他"}, {"Settings", "設定"},
        {"UI demo only - nothing here touches any game.", "UIデモ専用 - ゲームには一切接続しません。"},
        {"Preview canvas, crosshair and settings (everything is drawn inside Hatenium).", "プレビューキャンバスと設定(描画はすべてHatenium内部のみ)。"},
        {"Movement presets and indicators (demo only).", "移動プリセットとインジケーター(デモ用)。"},
        {"Interface extras and notifications.", "インターフェースの追加機能と通知。"},
        {"Appearance, window, application and configuration.", "外観、ウィンドウ、アプリ、設定ファイル。"},
        // common
        {"Enable", "有効"}, {"Keybind", "キーバインド"}, {"None", "なし"}, {"Color", "色"}, {"Opacity", "不透明度"},
        {"Size", "サイズ"}, {"Thickness", "太さ"}, {"Style", "スタイル"},
        // combat
        {"Aimbot", "エイムボット"}, {"FOV", "FOV"}, {"Smoothness", "スムーズさ"}, {"Target Selection", "ターゲット選択"},
        {"Closest", "最も近い"}, {"Head", "頭"}, {"Chest", "胸"}, {"Body", "体"},
        {"Triggerbot", "トリガーボット"}, {"Delay", "ディレイ"}, {"Hit Chance", "ヒット率"},
        {"Recoil Control", "リコイル制御"}, {"Horizontal", "水平"}, {"Vertical", "垂直"},
        {"Other Combat", "その他の戦闘設定"}, {"No Recoil", "リコイルなし"}, {"No Spread", "拡散なし"}, {"Instant Reload", "即時リロード"},
        // visuals
        {"Preview", "プレビュー"}, {"FOV Circle", "FOVサークル"}, {"Radius", "半径"}, {"Filled Circle", "塗りつぶし"},
        {"Outline", "アウトライン"}, {"Center X", "中心X"}, {"Center Y", "中心Y"},
        {"Crosshair", "クロスヘア"}, {"Gap", "ギャップ"}, {"Cross", "十字"}, {"Dot", "ドット"}, {"Circle", "円"}, {"Cross + Dot", "十字+ドット"},
        {"ESP Settings", "ESP設定"}, {"Box ESP", "ボックスESP"}, {"Name ESP", "名前ESP"}, {"Distance ESP", "距離ESP"}, {"Snaplines", "スナップライン"},
        {"Settings only - nothing is drawn or detected.", "設定のみ - 描画や検出は行いません。"},
        // visuals extras
        {"Screen Overlay", "画面オーバーレイ"}, {"Enable Overlay", "オーバーレイを有効化"}, {"Overlay Monitor", "表示するモニター"},
        {"Primary Monitor", "メインモニター"}, {"All Monitors", "すべてのモニター"},
        {"Draws on top of your screen (click-through). Only pixels are drawn - Hatenium never touches other programs.", "画面の最前面に描画します(クリックは透過)。描画するだけで、他のプログラムには一切触れません。"},
        {"Mouse Trail", "マウスの軌跡"}, {"Click Particles", "クリックパーティクル"}, {"Color Mode", "カラーモード"}, {"Color 2", "色2"},
        {"Line", "線"}, {"Dots", "点"}, {"Comet", "彗星"}, {"Solid", "単色"}, {"Gradient", "グラデーション"}, {"Rainbow", "レインボー"}, {"Random", "ランダム"},
        {"Rainbow Speed", "レインボー速度"}, {"Lifetime", "寿命"}, {"Taper", "先細り"}, {"Glow", "グロー"},
        {"On Left Click", "左クリック時"}, {"On Right Click", "右クリック時"}, {"Count", "数"}, {"Shape", "形"},
        {"Square", "スクエア"}, {"Star", "星"}, {"Ring", "リング"}, {"Speed", "速度"}, {"Gravity", "重力"},
        {"HUD", "HUD"}, {"Music", "ミュージック"}, {"Local", "ローカル"}, {"YouTube", "YouTube"},
        {"Crosshair, FOV circle, mouse trail and click particles - live preview and real screen overlay.", "クロスヘア、FOVサークル、マウスの軌跡、クリックパーティクル - リアルタイムプレビューと実際の画面オーバーレイ。"},
        {"Key strokes and CPS counter drawn by the overlay.", "キーストロークとCPSカウンターをオーバーレイに表示します。"},
        {"Play your own music files from this PC.", "このPCの音楽ファイルを再生します。"},
        {"HUD Overlay", "HUDオーバーレイ"}, {"CPS Counter", "CPSカウンター"}, {"Key Overlay", "キーオーバーレイ"}, {"Keys", "キー"}, {"Optional Keys", "オプションキー"},
        {"HUD elements are drawn by the screen overlay. Inputs are only polled for display - nothing is hooked, injected or stored.", "HUDは画面オーバーレイが描画します。入力は表示のために状態を読むだけで、フック・インジェクション・保存は行いません。"},
        {"Show Max CPS", "最大CPSを表示"}, {"Reset CPS", "CPSをリセット"}, {"Scale", "スケール"}, {"Position X", "位置X"}, {"Position Y", "位置Y"},
        {"Show Labels", "ラベルを表示"}, {"Active Color", "押下時の色"}, {"Inactive Color", "通常時の色"}, {"Text Color", "文字色"}, {"Choose which keys are shown.", "表示するキーを選びます。"},
        {"Space", "スペース"}, {"Shift", "Shift"}, {"Ctrl", "Ctrl"}, {"Mouse Left", "マウス左"}, {"Mouse Right", "マウス右"}, {"R (Reload)", "R(リロード)"},
        {"Mouse Side 1", "マウスサイド1"}, {"Mouse Side 2", "マウスサイド2"},
        {"Crosshair Animation", "クロスヘアアニメーション"}, {"Spread on Shoot", "射撃で拡散"}, {"Shrink on Move", "移動で縮小"}, {"Amount", "量"}, {"Anim Speed", "アニメ速度"},
        {"Easing", "イージング"}, {"Linear", "リニア"}, {"Ease Out", "イーズアウト"},
        {"Shows on the screen overlay (left mouse button / W A S D are polled, not recorded).", "画面オーバーレイに表示されます(左クリックとWASDは状態を読むだけで記録しません)。"},
        {"Power", "電源"}, {"Prevent Sleep", "スリープ防止"},
        {"Keeps Windows and the display awake while Hatenium runs. No fake input is ever sent to any game.", "Hatenium実行中はWindowsと画面をスリープさせません。ゲームへの偽入力は一切送りません。"},
        {"Now Playing", "再生中"}, {"Playlist", "プレイリスト"}, {"Nothing playing", "再生していません"}, {"Prev", "前へ"}, {"Next", "次へ"}, {"Play", "再生"}, {"Pause", "一時停止"},
        {"Volume", "音量"}, {"Shuffle", "シャッフル"}, {"Repeat", "リピート"}, {"Off", "オフ"}, {"All", "すべて"}, {"One", "1曲"},
        {"Add Files", "ファイルを追加"}, {"Add Folder", "フォルダーを追加"}, {"Clear", "クリア"}, {"Remove", "削除"},
        {"Supports MP3, WAV and FLAC. You can also drag and drop files or folders onto the window.", "MP3 / WAV / FLAC に対応。ファイルやフォルダーをウィンドウにドラッグ&ドロップでも追加できます。"},
        {"The playlist is empty.", "プレイリストは空です。"}, {"No audio output device available.", "音声出力デバイスが見つかりません。"}, {"tracks added", "曲を追加しました"},
        {"YouTube Player", "YouTubeプレイヤー"}, {"Open YouTube Music", "YouTube Musicを開く"}, {"Open YouTube", "YouTubeを開く"},
        {"Search Music", "音楽を検索"}, {"Search Videos", "動画を検索"}, {"Search YouTube...", "YouTubeを検索..."}, {"Close Player", "プレイヤーを閉じる"},
        {"The player window is closed.", "プレイヤーウィンドウは閉じています。"},
        {"Opens YouTube in its own window (a WebView2 browser). Sign in there - with a Premium account playback is ad-free. Hatenium only controls playback.", "YouTubeを専用ウィンドウ(WebView2ブラウザ)で開きます。そこでログインしてください。Premiumアカウントなら広告なしで再生されます。Hateniumは再生操作のみを行います。"},
        {"Tools", "ツール"}, {"Timer, stopwatch, play time and server region.", "タイマー、ストップウォッチ、プレイ時間、サーバー地域。"},
        {"Edit Layout", "レイアウトを編集"}, {"Theme", "テーマ"}, {"HUD Theme", "HUDテーマ"}, {"Border Width", "枠線の太さ"}, {"Background", "背景色"},
        {"New theme name", "新しいテーマ名"}, {"Save as Theme", "テーマとして保存"}, {"Delete Theme", "テーマを削除"}, {"Export Theme", "テーマをエクスポート"}, {"Import Theme", "テーマをインポート"},
        {"Sets background, text, accent colour and border width of the HUD.", "HUDの背景色・文字色・アクセント色・枠線の太さを設定します。"},
        {"Edit mode: drag the HUD elements on your screen. They snap to the edges and the centre. Ctrl + mouse wheel resizes the element under the cursor. Positions are remembered per monitor resolution (layout.json).", "編集モード: 画面上のHUDをドラッグして配置します。端と中央に吸着し、Ctrl+マウスホイールでカーソル下の要素を拡大縮小できます。位置はモニター解像度ごとに記憶されます(layout.json)。"},
        {"Watermark Image", "ウォーターマーク画像"}, {"Path to a PNG / JPEG image", "PNG / JPEG 画像のパス"}, {"Browse Image", "画像を選択"},
        {"Timer HUD", "タイマーHUD"}, {"Shows the timer / stopwatch from the Tools tab on the overlay.", "ツールタブのタイマー/ストップウォッチをオーバーレイに表示します。"},
        {"Presets", "プリセット"}, {"Export Preset", "プリセットをエクスポート"}, {"Import Preset", "プリセットをインポート"},
        {"Share your crosshair, FOV circle, trail, particles and HUD as one .json file.", "クロスヘア・FOVサークル・軌跡・パーティクル・HUDを1つの.jsonで共有できます。"},
        {"Preset exported", "プリセットをエクスポートしました"}, {"Preset imported", "プリセットをインポートしました"}, {"Theme exported", "テーマをエクスポートしました"}, {"Theme imported", "テーマをインポートしました"},
        {"Layout saved", "レイアウトを保存しました"}, {"Layout edit mode: drag the HUD on your screen", "レイアウト編集: 画面上のHUDをドラッグしてください"},
        {"Timer finished!", "タイマーが終了しました!"}, {"Profile", "プロファイル"}, {"Profile off - back to your setup", "プロファイル解除 - 元の設定に戻しました"},
        {"Timer", "タイマー"}, {"Stopwatch", "ストップウォッチ"}, {"Minutes", "分"}, {"Seconds", "秒"}, {"Start", "開始"}, {"Stop", "停止"}, {"Reset", "リセット"},
        {"Turn on \"Timer HUD\" in the HUD tab to show the running time on your screen.", "HUDタブの「タイマーHUD」を有効にすると、時間を画面に表示できます。"},
        {"Play Time", "プレイ時間"}, {"Track Play Time", "プレイ時間を記録"}, {"Recording now", "記録中"}, {"Tracked programs", "記録対象のプログラム"}, {"Add Program", "プログラムを追加"},
        {"Add program, e.g. game.exe", "プログラムを追加(例: game.exe)"},
        {"Measures how long the listed programs are running (a local playtime.json - nothing is sent anywhere).", "リストのプログラムの起動時間を計測します(ローカルのplaytime.jsonのみ。どこにも送信しません)。"},
        {"Server Region", "サーバー地域"}, {"Check Now", "今すぐ確認"}, {"Server IP", "サーバーIP"}, {"Estimated region", "推定地域"}, {"Unknown", "不明"},
        {"Online IP Lookup", "オンラインIP照会"},
        {"Reads the Roblox client log (a plain file read) and finds the location of the server IP. With Online IP Lookup on, only that server IP address is sent to ipwho.is (nothing else, nothing to the developer); a local geoip.csv is used first when present. This is only an estimate of the connection region, not an exact location or ping.", "Robloxのクライアントログ(通常のファイル読み取り)からサーバーIPを取り出し、その場所を特定します。オンラインIP照会がオンのときは、そのサーバーIPだけをipwho.isに送ります(それ以外は送らず、開発者にも送信しません)。ローカルのgeoip.csvがあれば先に使います。接続地域の推定であり、正確な位置やPingではありません。"},
        {"Looking up the location...", "場所を照会中..."}, {"Online lookup failed - check your internet connection", "オンライン照会に失敗しました - ネット接続を確認してください"},
        {"Estimated from the IP address (online lookup) - not an exact location or ping", "IPアドレスからの推定(オンライン照会)- 正確な位置やPingではありません"},
        {"Online lookup is off and no geoip.csv was found", "オンライン照会がオフで、geoip.csvも見つかりません"},
        {"Game Profiles", "ゲーム別プロファイル"}, {"No profiles yet.", "プロファイルはまだありません。"}, {"Profile name", "プロファイル名"},
        {"Program, e.g. RobloxPlayerBeta.exe", "プログラム(例: RobloxPlayerBeta.exe)"}, {"Pick a running program", "実行中のプログラムから選択"},
        {"Add Profile", "プロファイルを追加"}, {"Add Roblox", "Robloxを追加"}, {"Profile Enabled", "プロファイル有効"}, {"Save Current to Profile", "現在の設定を保存"}, {"Delete Profile", "プロファイルを削除"},
        {"While a listed program is running, its crosshair, HUD and keybinds are applied automatically. Hatenium only looks at the list of running program names.", "リストのプログラムが起動中は、そのクロスヘア・HUD・キーバインドを自動で適用します。Hateniumは実行中のプログラム名の一覧を見るだけです。"},
        {"Could not open the preset file", "プリセットファイルを開けません"}, {"The file is not valid JSON", "有効なJSONではありません"}, {"This is not a Hatenium preset", "Hateniumのプリセットではありません"},
        {"The preset contains invalid values", "プリセットに不正な値が含まれています"}, {"This is not a Hatenium theme", "Hateniumのテーマではありません"},
        {"Always on Top", "常に最前面"},
        // movement
        {"Bunny Hop", "バニーホップ"}, {"Auto Strafe", "オートストレイフ"}, {"Speed Indicator", "速度インジケーター"},
        {"Movement Preset", "移動プリセット"}, {"Default", "標準"}, {"Smooth", "スムーズ"}, {"Aggressive", "アグレッシブ"}, {"Custom", "カスタム"},
        {"Simulated speed", "シミュレーション速度"},
        // misc
        {"Interface", "インターフェース"}, {"Watermark", "ウォーターマーク"}, {"Animated Accent", "アニメーションアクセント"},
        {"FPS Counter", "FPSカウンター"}, {"Notification System", "通知システム"}, {"UI Animation", "UIアニメーション"},
        {"Panel Opacity", "パネル不透明度"}, {"Test Notification", "通知をテスト"}, {"This is a test notification", "これはテスト通知です"},
        // settings
        {"Appearance", "外観"}, {"Theme Color", "テーマカラー"}, {"Custom Color", "カスタムカラー"}, {"Button Shape", "ボタン形状"},
        {"Window Appearance", "ウィンドウ外観"}, {"UI Scale", "UIスケール"},
        {"Cyan", "シアン"}, {"Purple", "パープル"}, {"Red", "レッド"}, {"Green", "グリーン"}, {"White", "ホワイト"},
        {"Square", "スクエア"}, {"Slight", "わずか"}, {"Soft", "ソフト"}, {"Extra", "超丸"}, {"Slight Rounded", "わずかに丸"}, {"Rounded", "丸"}, {"Pill", "ピル"},
        {"Sharp", "シャープ"}, {"Soft Rounded", "ソフト"}, {"Extra Rounded", "超丸"},
        {"Window", "ウィンドウ"}, {"Window Width", "ウィンドウ幅"}, {"Window Height", "ウィンドウ高さ"},
        {"Remember Position", "位置を記憶"}, {"Remember Size", "サイズを記憶"},
        {"Application", "アプリケーション"}, {"Startup Animation", "起動アニメーション"}, {"Animation Speed", "アニメーション速度"},
        {"Language", "言語"}, {"Reset Settings", "設定をリセット"}, {"Reopen Setup", "セットアップを再表示"},
        {"Configuration", "設定ファイル"}, {"Save", "保存"}, {"Load", "読込"}, {"Reset", "リセット"}, {"Export", "エクスポート"}, {"Import", "インポート"},
        {"Reset everything to defaults?", "すべて初期設定に戻しますか?"}, {"Confirm", "実行"}, {"Cancel", "キャンセル"},
        // setup
        {"Appearance Setup", "外観セットアップ"}, {"Choose how Hatenium looks. You can change this any time in Settings.", "Hateniumの見た目を選びます。設定からいつでも変更できます。"},
        {"Live Preview", "リアルタイムプレビュー"}, {"Primary", "プライマリ"}, {"Secondary", "セカンダリ"}, {"Sample toggle", "サンプル"}, {"Sample slider", "サンプルスライダー"},
        {"Finish Setup", "セットアップ完了"},
        // sidebar / status
        {"Current Tab", "現在のタブ"}, {"Status", "状態"}, {"Version", "バージョン"}, {"Ready - UI demo", "準備完了 - UIデモ"},
        // splash
        {"Loading configuration", "設定を読み込み中"}, {"Applying theme", "テーマを適用中"}, {"Preparing interface", "インターフェースを準備中"}, {"Ready", "準備完了"},
        // notifications
        {"Settings saved", "設定を保存しました"}, {"Settings loaded", "設定を読み込みました"}, {"Settings reset", "設定をリセットしました"},
        {"Could not save settings", "設定を保存できませんでした"}, {"Could not read that file", "ファイルを読み込めませんでした"},
        {"Exported", "エクスポートしました"}, {"Imported", "インポートしました"}, {"Setup complete", "セットアップが完了しました"},
        {"enabled", "有効"}, {"disabled", "無効"},
        {"Config file was damaged - restored from backup", "設定ファイルが破損していたためバックアップから復元しました"},
        {"Config file was damaged - defaults loaded", "設定ファイルが破損していたため初期設定を読み込みました"},
    };
    return t;
}
} // namespace

void SetLanguage(int lang) { g_lang = lang; }
int Language() { return g_lang; }

const char* tr(const char* en) {
    if (g_lang == 0 || !en) return en;
    const auto& t = Table();
    auto it = t.find(en);
    return it == t.end() ? en : it->second;
}

} // namespace hat
