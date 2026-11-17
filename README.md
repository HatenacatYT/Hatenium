# Hatenium

Windows 10 / 11 向けの **Standalone Fake Cheat UI** アプリケーションです。
C++17 + Dear ImGui + DirectX 11 + Win32 API で作られており、**Visual Studio IDE は一切不要**です。
VS Code + MSYS2 UCRT64 + MinGW-w64 GCC + CMake + Ninja だけでビルド・デバッグできます。

> **設計方針:** Hatenium はゲームに一切アタッチしません。DLL インジェクション、プロセス操作、ゲームメモリへのアクセス、
> ゲーム入力への干渉、フックは **実装されていません**。画面オーバーレイは「透明・クリック透過のウィンドウに自分でピクセルを描く」だけで、
> カーソル位置・マウスボタン・WASD などのキー状態は `GetCursorPos` / `GetAsyncKeyState` で *読む* だけです(記録・送信はしません)。
> データ収集はなく、すべてローカルで動作します。ESP は設定 UI のみで、描画やターゲット検出は行いません。
> 一部のオンラインゲームではオーバーレイ自体が規約・アンチチートで問題になることがあります。ご自身の責任で使ってください。
> オーバーレイは「ボーダーレス / ウィンドウ表示」のゲームの上に表示されます(排他的フルスクリーンの上には出ません)。

## 機能

- Splash(16:9・角丸・最低2秒) → 初回のみ Appearance Setup(縦 9:16 へ滑らかに拡張) → Main GUI
- タブ: Combat / Visuals / Movement / Misc / Settings
- Visuals: FOV Circle・Crosshair のリアルタイムプレビュー、ESP 設定 UI
- **画面オーバーレイ(実動作):** FOV Circle / Crosshair / マウスの軌跡(色・グラデーション・レインボー・太さ・寿命・先細り・グロー)/ クリックパーティクル(形・色・数・速度・重力など)
- **HUD:** キーストローク表示(W A S D / Space / Shift / Ctrl / 左右クリック、任意で R E Q F とサイドボタン、色・位置・スケール設定)、CPS 表示(直近 1 秒・最大値・リセット)
- **HUD レイアウトエディタ:** 「Edit Layout」で画面上の HUD(キー・CPS・タイマー・ウォーターマーク)をドラッグ配置。端と中央に吸着し、**Ctrl + マウスホイール(または Ctrl + `+` / `-`)でカーソル下の要素を拡大縮小**、位置はモニター解像度ごとに `layout.json` へ保存
- **HUD テーマ:** Cyberpunk / Minimal / Gaming / Sunset と、自作テーマ(`themes.json`)。テーマ単体のエクスポート/インポートに対応
- **ウォーターマーク画像:** PNG / JPEG を HUD に表示(透明度・サイズ・位置)。**タイマー/ストップウォッチ**(Tools タブ)を画面に表示可能
- **ゲーム別プロファイル:** 実行中のプログラム名(例 `RobloxPlayerBeta.exe`)を見て、クロスヘア・HUD・キーバインドを自動で切り替え(Settings タブ)
- **プリセット共有:** クロスヘア・FOV・軌跡・パーティクル・HUD を 1 つの `.json` でエクスポート/インポート(形式・バージョン・値を検証)
- **プレイ時間トラッカー:** 指定プログラムの起動時間を日別に `playtime.json` へ記録し、棒グラフで表示(完全ローカル)
- **サーバー地域(推定):** Roblox のクライアントログから接続先 IP を読み、その場所を自動で特定します(オンライン IP 照会 / ローカル `geoip.csv`。下記)
- クロスヘアアニメーション(射撃で拡散 / 移動で縮小、イージングと速度)、スリープ防止(`SetThreadExecutionState`、偽入力なし)
- **Music Player:** ローカルの MP3 / WAV / FLAC(プレイリスト保存、シャッフル、リピート、シーク、ドラッグ&ドロップ)。オプションで YouTube(下記)
- テーマカラー(Cyan / Purple / Red / Green / White / Custom)、ボタン形状、ウィンドウ角丸を即時反映
- **常に最前面:** ヘッダーのピンボタン、または Settings → Window → Always on Top で Hatenium のウィンドウを他のウィンドウの上に固定(設定は保存されます)
- 英語 / 日本語切り替え、UI スケール、アニメーション速度
- Per-Monitor DPI 対応、画面端検出、解像度変更への追従、リサイズ可能なボーダーレスウィンドウ
- JSON 設定(原子的な書き込み、`.bak` バックアップ、破損時の自動復旧)、Export / Import

## 必要なもの

1. Visual Studio Code(拡張機能: **C/C++** `ms-vscode.cpptools`、**CMake Tools** `ms-vscode.cmake-tools`)
2. [MSYS2](https://www.msys2.org/)
3. MinGW-w64 UCRT64 GCC、CMake、Ninja、Git(下記コマンドでインストール)
4. インターネット接続(初回の CMake Configure で Dear ImGui・nlohmann/json・miniaudio を GitHub から取得します)

### MSYS2 の導入

1. https://www.msys2.org/ のインストーラーでインストールします(既定: `C:\msys64`)。
2. 「MSYS2 UCRT64」ターミナルを開き、次を実行します。

```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-gdb \
  git
```

`gdb` は VS Code でのデバッグ(F5)に使います。ビルドだけなら不要です。

### PATH 設定

PowerShell / コマンドプロンプトからビルドする場合は、次をユーザー環境変数 `Path` に追加します。

```text
C:\msys64\ucrt64\bin
C:\msys64\usr\bin
```

MSYS2 を別の場所にインストールした場合は、そのパスに置き換えてください。
VS Code のタスク / デバッグは `.vscode/settings.json` の `hatenium.msys2Root` を使うので、固定パスに依存しません
(別の場所なら `hatenium.msys2Root` だけ書き換えます)。

## ビルド

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

実行ファイルは `build/Hatenium.exe` に生成されます(Release、標準ライブラリを静的リンクした単体 exe)。
`build/config/` と `build/assets/` も自動で作られます。

### VS Code から

`Ctrl+Shift+B`(既定のビルドタスク = Release)、または「ターミナル → タスクの実行」で次を選びます。

| タスク | 内容 |
|---|---|
| CMake Configure (Release / Debug) | CMake の構成(`build/` / `build-debug/`) |
| Build Release / Build Debug | ビルド |
| Clean Build | `build/` と `build-debug/` を削除 |
| Run Application | Release ビルドして `build/Hatenium.exe` を起動 |

### デバッグ

`F5` → 「Debug Hatenium (GDB)」。Debug ビルド(`build-debug/Hatenium.exe`)を作って GDB で起動します。ブレークポイントが使えます。

### オフラインでビルドする場合

あらかじめ Dear ImGui(v1.91.5)と nlohmann/json(v3.11.3)を取得しておき、次のように指定します。

```powershell
cmake -S . -B build -G Ninja -DFETCHCONTENT_FULLY_DISCONNECTED=ON `
  -DFETCHCONTENT_SOURCE_DIR_IMGUI=C:\libs\imgui `
  -DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=C:\libs\json
```

## YouTube プレイヤー(オプション)

既定では **OFF** です。有効にすると、Music タブから YouTube / YouTube Music を専用ウィンドウ(WebView2 ブラウザ)で開き、
検索・再生/一時停止・次へ/前へ・音量・再生中の曲名表示を Hatenium から操作できます。YouTube の動画や音声を抜き出すことはせず、
本物の youtube.com を表示するだけです。**YouTube Premium のアカウントでそのウィンドウにログインすると広告なしで再生されます**
(ログイン状態は WebView2 が保存します)。広告ブロッカーの同梱や設定は行いません。

```powershell
cmake -S . -B build -G Ninja -DHATENIUM_ENABLE_YOUTUBE=ON
cmake --build build
```

- 初回の Configure で webview(MIT)と Microsoft WebView2 SDK のヘッダー(NuGet)をダウンロードします。取得できない場合は自動的に OFF になります。
- 実行には Microsoft Edge WebView2 Runtime が必要です(Windows 11 には標準で入っています)。
- この機能は GitHub Actions の `build-youtube` ジョブ(失敗しても全体は失敗になりません)でもビルドされます。

## GitHub Actions

`.github/workflows/build.yml` が `windows-latest` + MSYS2 UCRT64 で Release / Debug(と YouTube 付き)をビルドし、`Hatenium.exe` を Artifact として保存します。
`v0.1.0` のような **タグ** を push すると、Release ビルドを zip にして GitHub Release に添付します。

## サーバー地域表示

Tools タブの「Server Region」は、`%LOCALAPPDATA%\Roblox\logs` の最新ログ(通常のファイル読み取り)から接続先 IPv4 を取り出し、場所を特定します。

- **オンライン IP 照会(既定でオン):** サーバーの IP アドレスだけを `https://ipwho.is` に問い合わせて、都市・地域・国・組織名を表示します。
  それ以外の情報は送らず、開発者のサーバーへの送信は一切ありません。結果はキャッシュされ、バックグラウンドで実行されます。オフにすると通信しません。
- **ローカル `geoip.csv`(任意・優先):** `config/geoip.csv`(または `Hatenium.exe` と同じ場所 / `assets/geoip.csv`)に IP 範囲 CSV
  (1・2 列目が開始 IP・終了 IP、3 列目以降が地域名。例: DB-IP の Country Lite)を置くと、完全オフラインで引けます。
- ログの書式は Roblox 側の都合で変わることがあり、結果はあくまで「接続地域の推定」です(正確な位置や Ping ではありません)。

## 設定ファイル

`Hatenium.exe` と同じ場所の `config/hatenium.json`(書き込めない場所なら `%APPDATA%\Hatenium\`)。

- 同じ場所に `layout.json`(HUD 位置)、`themes.json`(自作テーマ)、`playtime.json`(プレイ時間)も保存されます。
- 書き込みは `.tmp` に書いてから置き換える方式で、直前の正常な設定を `hatenium.json.bak` に残します。
- 壊れていた場合は `hatenium.json.corrupt` に退避し、`.bak` から復旧(なければ初期設定)して通知します。
- 設定は変更から数秒後に自動保存されます。

## フォント

Windows 標準の Segoe UI(日本語時は Meiryo / Yu Gothic)を使います。
`assets/fonts/Hatenium-Regular.ttf` / `Hatenium-Bold.ttf` を置くと、そちらが優先されます。

## プロジェクト構成

```text
Hatenium/
├── CMakeLists.txt
├── .vscode/            tasks.json / launch.json / c_cpp_properties.json / settings.json
├── .github/workflows/  build.yml
├── src/
│   ├── main.cpp
│   ├── core/           Application / WindowManager / AnimationManager / ConfigManager / Settings / Localization
│   ├── audio/          MusicPlayer(miniaudio) / YouTubePlayer(WebView2, 任意)
│   ├── gui/            SplashScreen / SetupWindow / MainWindow / ThemeManager / Components / pages/*
│   └── rendering/      Renderer (D3D11) / PreviewRenderer / OverlayWindow(GDI+) / Effects / Hud
├── assets/fonts/
└── config/
```

## トラブルシューティング

- **`cmake` が見つからない:** MSYS2 の `ucrt64\bin` が PATH に入っているか、「MSYS2 UCRT64」ターミナルで実行しているか確認してください。
- **GitHub から取得できない:** プロキシ / ネットワーク設定を確認するか、上記「オフラインでビルドする場合」を使ってください。
- **Direct3D 11 を初期化できない:** GPU ドライバーを更新してください(GPU が無い環境では WARP ソフトウェア描画に自動で切り替わります)。
