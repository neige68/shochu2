// <parse.cpp>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief parse
//

#include "parse.h"

// Boost.JSON をヘッダオンリーで使う(この翻訳単位のみ)
#include <boost/json/src.hpp>

#include <algorithm>           // std::count
#include <cctype>              // std::isalnum
#include <sstream>             // std::istringstream

namespace shochu {

std::string base64_encode(const std::string& data)
{
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        unsigned v = (static_cast<unsigned char>(data[i]) << 16) |
                     (static_cast<unsigned char>(data[i + 1]) << 8) |
                     static_cast<unsigned char>(data[i + 2]);
        out += table[(v >> 18) & 63];
        out += table[(v >> 12) & 63];
        out += table[(v >> 6) & 63];
        out += table[v & 63];
    }
    if (i + 1 == data.size()) {
        unsigned v = static_cast<unsigned char>(data[i]) << 16;
        out += table[(v >> 18) & 63];
        out += table[(v >> 12) & 63];
        out += "==";
    }
    else if (i + 2 == data.size()) {
        unsigned v = (static_cast<unsigned char>(data[i]) << 16) |
                     (static_cast<unsigned char>(data[i + 1]) << 8);
        out += table[(v >> 18) & 63];
        out += table[(v >> 12) & 63];
        out += table[(v >> 6) & 63];
        out += '=';
    }
    return out;
}

bool is_ipv4(const std::string& s)
{
    int parts = 0;
    size_t i = 0;
    while (i <= s.size()) {
        size_t start = i;
        int value = 0;
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
            value = value * 10 + (s[i] - '0');
            if (value > 255) return false;
            ++i;
        }
        size_t len = i - start;
        if (len == 0 || len > 3) return false;
        if (len > 1 && s[start] == '0') return false; // 先頭ゼロ不可
        ++parts;
        if (i == s.size()) break;
        if (s[i] != '.') return false;
        ++i;
        if (i == s.size()) return false; // 末尾が '.'
    }
    return parts == 4;
}

bool is_valid_fqdn(const std::string& s)
{
    if (s.empty() || s.size() > 253) return false;
    for (char c : s) {
        unsigned char u = static_cast<unsigned char>(c);
        if (!(u < 0x80 && std::isalnum(u)) && c != '.' && c != '-' && c != '_' && c != '*')
            return false;
    }
    return s.front() != '.' && s.back() != '.' && s.find("..") == std::string::npos;
}

std::optional<std::string> parse_trace_ip(const std::string& body)
{
    std::istringstream iss(body);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.compare(0, 3, "ip=") == 0) {
            std::string ip = line.substr(3);
            if (is_ipv4(ip)) return ip;
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::vector<std::string> zone_candidates(const std::string& fqdn)
{
    std::vector<std::string> result;
    std::string rest = fqdn;
    while (std::count(rest.begin(), rest.end(), '.') >= 1) {
        result.push_back(rest);
        rest = rest.substr(rest.find('.') + 1);
    }
    return result;
}

namespace {

// JSON を解析して object を返す。失敗したら nullopt
std::optional<boost::json::object> parse_object(const std::string& json)
{
    boost::system::error_code ec;
    boost::json::value v = boost::json::parse(json, ec);
    if (ec || !v.is_object()) return std::nullopt;
    return v.as_object();
}

// success == true で result が空でない配列なら result[0] を返す
const boost::json::object* first_result(const boost::json::object& root)
{
    auto success = root.if_contains("success");
    if (!success || !success->is_bool() || !success->as_bool()) return nullptr;
    auto result = root.if_contains("result");
    if (!result || !result->is_array() || result->as_array().empty()) return nullptr;
    const auto& first = result->as_array().front();
    return first.is_object() ? &first.as_object() : nullptr;
}

std::optional<std::string> string_member(const boost::json::object& o, const char* key)
{
    auto v = o.if_contains(key);
    if (!v || !v->is_string()) return std::nullopt;
    return std::string(v->as_string().c_str());
}

} // namespace

std::string cf_error_message(const std::string& json)
{
    auto root = parse_object(json);
    if (!root) return "invalid JSON";
    std::string msg;
    auto errors = root->if_contains("errors");
    if (errors && errors->is_array()) {
        for (const auto& e : errors->as_array()) {
            if (!e.is_object()) continue;
            if (!msg.empty()) msg += "; ";
            auto code = e.as_object().if_contains("code");
            if (code && code->is_int64()) msg += std::to_string(code->as_int64()) + ": ";
            msg += string_member(e.as_object(), "message").value_or("");
        }
    }
    return msg;
}

std::optional<std::string> parse_cf_first_id(const std::string& json)
{
    auto root = parse_object(json);
    if (!root) return std::nullopt;
    auto first = first_result(*root);
    if (!first) return std::nullopt;
    return string_member(*first, "id");
}

std::optional<CfRecord> parse_cf_record(const std::string& json)
{
    auto root = parse_object(json);
    if (!root) return std::nullopt;
    auto first = first_result(*root);
    if (!first) return std::nullopt;
    auto id = string_member(*first, "id");
    auto content = string_member(*first, "content");
    if (!id || !content) return std::nullopt;
    return CfRecord{*id, *content};
}

std::string cf_patch_body(const std::string& ip)
{
    return "{\"content\":\"" + ip + "\"}";
}

} // namespace shochu

// end of <parse.cpp>
