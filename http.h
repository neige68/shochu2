// <http.h>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief http (WinHTTP wrapper)
//

#ifndef http_h
#define http_h

#include <string>               // std::string
#include <vector>               // std::vector

/// HTTP 応答
struct HttpResponse {
    int status = 0;             ///< HTTP ステータスコード
    std::string body;           ///< 本文(最大 1MB)
};

/// HTTPS リクエストを送って応答を受け取る
//
/// \param[in] method       "GET", "PATCH" など
/// \param[in] url          https:// で始まる URL(http:// は認証情報の漏洩防止のため拒否する)
/// \param[in] headers      "Name: value" 形式のヘッダ行
/// \param[in] body         リクエスト本文(空なら無し)
/// \param[in] timeout_sec  名前解決・接続・送信・受信それぞれのタイムアウト[秒]
//
/// 通信エラーのときは std::runtime_error を送出する。HTTP のエラー応答(4xx/5xx)は例外にしない。
HttpResponse http_request(const std::string& method, const std::string& url,
                          const std::vector<std::string>& headers,
                          const std::string& body, int timeout_sec);

#endif // http_h

// end of <http.h>
