// <parse.h>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief ネットワークに依存しない純粋関数（単体テスト対象）
//

#ifndef parse_h
#define parse_h

#include <optional>             // std::optional
#include <string>               // std::string
#include <vector>               // std::vector

namespace shochu {

/// Base64 エンコード(RFC 4648, パディングあり)
std::string base64_encode(const std::string& data);

/// "a.b.c.d" 形式の IPv4 アドレスか(各 0-255, 先頭ゼロ・余分な文字は不可)
bool is_ipv4(const std::string& s);

/// DNS 名として扱ってよい文字だけか(英数字 . - _ * のみ, 1-253 文字)
//
/// Cloudflare API の URL に埋め込むため、それ以外は拒否する
bool is_valid_fqdn(const std::string& s);

/// cdn-cgi/trace の応答から "ip=" 行を取り出す。IPv4 でなければ nullopt
std::optional<std::string> parse_trace_ip(const std::string& body);

/// "a.b.example.com" -> {"a.b.example.com", "b.example.com", "example.com"}
//
/// ゾーン名の候補(長い順)。ラベルが 2 個未満になるものは含めない
std::vector<std::string> zone_candidates(const std::string& fqdn);

/// Cloudflare の応答に含まれるエラーを "code: message; ..." にまとめる(無ければ空)
std::string cf_error_message(const std::string& json);

/// 応答が success=true で result[0].id があればその id (ゾーン検索用)
std::optional<std::string> parse_cf_first_id(const std::string& json);

/// DNS レコード(A)
struct CfRecord {
    std::string id;
    std::string content;
};

/// 応答が success=true で result[0] に id と content があれば返す
std::optional<CfRecord> parse_cf_record(const std::string& json);

/// PATCH の本文: {"content":"<ip>"} (ip は is_ipv4 であること)
std::string cf_patch_body(const std::string& ip);

} // namespace shochu

#endif // parse_h

// end of <parse.h>
