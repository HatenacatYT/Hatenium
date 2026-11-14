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
