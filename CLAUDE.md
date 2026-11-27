# CLAUDE.md

shochu2 = MyDNS.JP と Cloudflare DNS (A レコード) に現在のグローバル IPv4 アドレスを通知・更新する Windows サービス。
旧 shochu (BCB5) の VC 移植 + Cloudflare 対応。C++20 / MSVC Win32 (VS2019 = `VC142`) / Boost 1.88 (Log 静的, JSON・Test ヘッダのみ) / WinHTTP。
public リポジトリ（ライセンスは MIT。`LICENSE` に著作権表示あり）。サブモジュールなし。

## ビルド・テスト

- `build.bat [cm] [rebuild] [d] [r]` — cmake → msbuild → ctest。出力末尾の `BUILD SUCCEEDED.` / `BUILD FAILED.` で判定（exit code は信頼しない）
- WSL からは `bash /mnt/tmp/shochu2/run_build.sh [args]`（グローバル設定の build.bat 手順参照。ログは `tail ---disable-inotify`）
- 単体テストは `test/test_shochu.cpp`（`parse.cpp` の純粋関数のみ。ネットワーク不要）
- 実機確認: `build\Debug\shochu.exe -debug`（exe と同じディレクトリに `shochu.ini` が必要）

## 構成

- `service.cpp/h` — Microsoft サービスサンプル由来（hslss と同じ）。`-install` / `-remove` / `-debug`。
  `RegisterServiceCtrlHandlerEx` + `SERVICE_ACCEPT_POWEREVENT` で、スリープ復帰（`PBT_APMRESUMEAUTOMATIC` / `PBT_APMRESUMESUSPEND`）時に `ServiceResume()` を呼ぶ
- `start.cpp` — `ServiceStart` / `ServiceStop` / `ServiceResume`。停止イベントと復帰イベントを `WaitForMultipleObjects` で待つループ。周期ごとに `Config::load()`。
  待機タイムアウトはスリープ中の時間を数えないため、復帰通知で待ちを打ち切り `RESUME_DELAY`（10 秒。ネットワーク復旧待ち）後に 1 周期実行する
- `mydns.cpp` — `[MYDNS]` の各アカウントで Basic 認証 GET
- `cloudflare.cpp` — IP 取得 → ゾーン検索 → A レコード検索 → 差分があれば PATCH。`RecheckCycles`（既定 1 = 毎周期）ごとにレコードを再確認する。トークンは `ApiToken` 1 つのみ（全レコード共通）
- `http.cpp` — WinHTTP ラッパー（https のみ許可）
- `parse.cpp` — Base64 / IPv4 検証 / trace 解析 / Cloudflare JSON 解析（テスト対象）

## リリース手順

1. `version.h` の `VER_PRODUCTVERSION_STR` / `VER_PRODUCTVERSION` / `VER_FILEVERSION_STR` / `VER_FILEVERSION` を更新
2. `build.bat r`（`BUILD SUCCEEDED.` と ctest の結果を確認）→ `dist.bat`（`build\dist\shochu2.7z` に `shochu.exe` / `shochu.ini.sample` / `LICENSE`）
3. コミット → `git tag vX.Y.Z` → `git push origin master vX.Y.Z`
4. `gh release create vX.Y.Z build/dist/shochu2.7z --title vX.Y.Z --notes-file <ノート>`（public なので認証なしで `releases/latest/download/shochu2.7z` から取得できる）

## 注意

- **秘密情報（`shochu.ini` の ID・パスワード・API トークン）をコミット・ログ出力しない**。サンプルは `shochu.ini.sample`
- ini のコメントは行頭のみ（Boost.PropertyTree は行末コメント非対応）
- ソース (`.cpp`/`.h`) は BOM 付き UTF-8 + CRLF（Write/Edit 後に Python で変換）。`build.bat` / `.rc` / `.ini.sample` は ASCII + CRLF
- git 操作は `/mnt/github/` 配下のため Windows の git.exe（PowerShell 経由）を使う
