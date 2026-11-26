# shochu2 - MyDNS / Cloudflare 動的 DNS 更新サービス

自宅や事業所のグローバル IPv4 アドレスを定期的に **MyDNS.JP** に通知し、
さらに **Cloudflare DNS の A レコード** を書き換える Windows サービスです。

旧 `shochu`（Borland C++ Builder 5、POP3 で MyDNS に通知。2009-2013）を
Visual C++ でビルドできるように作り直したものです。

- MyDNS.JP: `https://ipv4.mydns.jp/login.html` に Basic 認証でアクセス（POP3 は廃止）
- Cloudflare: 現在の IPv4 アドレスを `https://1.1.1.1/cdn-cgi/trace` で調べ、
  設定したレコードの content が異なるときだけ API で更新（proxied / TTL は変更しない）
- 通信は WinHTTP（HTTPS のみ。OS の TLS を使用）
- 正常時 15 分、異常時 1 分間隔（`shochu.ini` で変更可）

## 必要なもの

- Windows 8 以上
- ビルド: Visual Studio 2019 (VC142)、CMake 3.28 以上、Boost 1.88 以上（Log を静的リンク。JSON・Test はヘッダのみ）

## ビルド

```
build.bat [cm] [rebuild] [d] [r]
```

出力の最後の行が `BUILD SUCCEEDED.` / `BUILD FAILED.` です。単体テスト（`test_shochu`）も実行されます。
Boost が C: 以外にある場合は環境変数 `CMAKE_PREFIX_PATH` で指定してください。

## インストール

### 入手

[Releases](https://github.com/neige68/shochu2/releases/latest) ページから `shochu2.7z` をダウンロードし、
7-Zip などで展開します（`shochu.exe` / `shochu.ini.sample` / `LICENSE` が入っています）。

コマンドで取得する場合（どれか 1 つ）:

```
curl.exe -L -O https://github.com/neige68/shochu2/releases/latest/download/shochu2.7z
```

```
powershell -Command "Invoke-WebRequest https://github.com/neige68/shochu2/releases/latest/download/shochu2.7z -OutFile shochu2.7z"
```

```
gh release download --repo neige68/shochu2 --pattern shochu2.7z
```

`curl.exe` は Windows 10 以降に標準で入っています。展開は `7z x shochu2.7z`。

### 新規インストール

1. `shochu.exe` を任意のディレクトリに置き、同じディレクトリで `shochu.ini.sample` を `shochu.ini` にコピーして編集します。
   **`shochu.ini` にはパスワードや API トークンが入るのでコミットしないでください**（`.gitignore` 済み）
2. 管理者権限のコマンドプロンプトで

   ```
   shochu.exe -install
   net start shochu
   ```

   サービス名は `shochu`、スタートアップの種類は自動（`Tcpip` に依存）です。

### 旧 shochu（BCB5 版）からの置き換え

サービス名が同じ `shochu` なので、アンインストール・再インストールは不要です。
管理者権限のコマンドプロンプトで、サービスを止めて exe を入れ替え、再開します
（旧 exe からの入れ替えで動作することを確認済み）。

```
net stop shochu
copy /y shochu.exe <旧 shochu.exe のあるディレクトリ>\shochu.exe
net start shochu
```

- 旧版の `shochu.ini`（`[POP3]` 形式）は**そのままでは使えません**。
  `[MYDNS]` / `[CLOUDFLARE]` のどちらも有効でないと、警告をログに出して 1 分ごとに再試行するだけで、何も通知しません。
  `shochu.ini.sample` を参考に `[MYDNS]`（`Enable=1`、`User0` / `Password0`）へ書き直してください。
  ini は周期ごとに読み直すので、書き直せば再起動なしで有効になります。
- 旧版の認証情報はこのリポジトリに含まれていません。手元の旧 ini から移してください。

## アンインストール

```
shochu.exe -remove
```

## 設定（shochu.ini）

`shochu.ini.sample` を参照してください。`shochu.ini` は周期ごとに読み直すので、変更にサービスの再起動は不要です。
ini のコメントは**行頭のみ**です（行末コメントは値に含まれてしまいます）。

### Cloudflare の API トークン

Cloudflare ダッシュボードで、対象ゾーンに対する `Zone : Zone : Read` と `Zone : DNS : Edit` 権限だけを持つ
API トークンを作成してください。更新対象の A レコードは**あらかじめ作成**しておきます（無いレコードは作成しません）。

## ログ

`LogDir`（既定は exe と同じディレクトリの `log`）に `shochu_YYYYMM.log` が月ごとにできます。
パスワードと API トークンは出力しません。

## デバッグ実行

```
shochu.exe -debug
```

コンソールで実行し、Ctrl+C で停止します（ログもコンソールに出ます）。

## ライセンス

MIT License（`LICENSE` を参照）。`service.cpp` / `service.h` は Microsoft の
Windows Classic Samples（MIT License）をもとにしています。
