# Hatenium

Windows 10 / 11 向けの **Standalone Fake Cheat UI** アプリケーションです。
C++17 + Dear ImGui + DirectX 11 + Win32 API で作られており、**Visual Studio IDE は一切不要**です。
VS Code + MSYS2 UCRT64 + MinGW-w64 GCC + CMake + Ninja だけでビルド・デバッグできます。

> **重要:** Hatenium は UI デモ専用です。ゲームへの接続、DLL インジェクション、プロセス操作、ゲームメモリへのアクセス、
> ゲーム入力への干渉は **一切実装されていません**。FOV Circle / Crosshair は Hatenium 自身のプレビューキャンバスの中にだけ描画されます。
> ESP は設定 UI のみで、描画やターゲット検出は行いません。

## 機能

- Splash(16:9・角丸・最低2秒) → 初回のみ Appearance Setup(縦 9:16 へ滑らかに拡張) → Main GUI
- タブ: Combat / Visuals / Movement / Misc / Settings
- Visuals: FOV Circle・Crosshair のリアルタイムプレビュー、ESP 設定 UI
- テーマカラー(Cyan / Purple / Red / Green / White / Custom)、ボタン形状、ウィンドウ角丸を即時反映
- 英語 / 日本語切り替え、UI スケール、アニメーション速度
- Per-Monitor DPI 対応、画面端検出、解像度変更への追従、リサイズ可能なボーダーレスウィンドウ
- JSON 設定(原子的な書き込み、`.bak` バックアップ、破損時の自動復旧)、Export / Import

## 必要なもの

1. Visual Studio Code(拡張機能: **C/C++** `ms-vscode.cpptools`、**CMake Tools** `ms-vscode.cmake-tools`)
2. [MSYS2](https://www.msys2.org/)
3. MinGW-w64 UCRT64 GCC、CMake、Ninja、Git(下記コマンドでインストール)
4. インターネット接続(初回の CMake Configure で Dear ImGui と nlohmann/json を GitHub から取得します)

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

## GitHub Actions

`.github/workflows/build.yml` が `windows-latest` + MSYS2 UCRT64 で Release / Debug をビルドし、`Hatenium.exe` を Artifact として保存します。
`v0.1.0` のような **タグ** を push すると、Release ビルドを zip にして GitHub Release に添付します。

## 設定ファイル

`Hatenium.exe` と同じ場所の `config/hatenium.json`(書き込めない場所なら `%APPDATA%\Hatenium\`)。

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
│   ├── gui/            SplashScreen / SetupWindow / MainWindow / ThemeManager / Components / pages/*
│   └── rendering/      Renderer (D3D11) / PreviewRenderer
├── assets/fonts/
└── config/
```

## トラブルシューティング

- **`cmake` が見つからない:** MSYS2 の `ucrt64\bin` が PATH に入っているか、「MSYS2 UCRT64」ターミナルで実行しているか確認してください。
- **GitHub から取得できない:** プロキシ / ネットワーク設定を確認するか、上記「オフラインでビルドする場合」を使ってください。
- **Direct3D 11 を初期化できない:** GPU ドライバーを更新してください(GPU が無い環境では WARP ソフトウェア描画に自動で切り替わります)。
