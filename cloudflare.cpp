// <cloudflare.cpp>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief Cloudflare DNS の A レコード更新
//

#include "pch.h"
#pragma hdrstop

#include "cloudflare.h"

#include "http.h"               // http_request
#include "log.h"                // BOOST_LOG_TRIVIAL
#include "parse.h"              // shochu::parse_trace_ip

#include <exception>            // std::exception
#include <stdexcept>            // std::runtime_error

using namespace std;

namespace {

const char* const API_BASE = "https://api.cloudflare.com/client/v4";

} // namespace

// Cloudflare API を呼ぶ。HTTP 200 以外は例外。応答本文を返す
string Cloudflare::api(const Config& config, const string& method, const string& path, const string& body)
{
    auto res = http_request(method, API_BASE + path,
                            {"Authorization: Bearer " + config.cf_api_token(),
                             "Content-Type: application/json"},
                            body, config.http_timeout());
    if (res.status != 200) {
        string msg = shochu::cf_error_message(res.body);
        throw runtime_error("Cloudflare API " + method + ' ' + path + " returned HTTP " + to_string(res.status)
                            + (msg.empty() ? "" : " (" + msg + ")"));
    }
    return res.body;
}

// FQDN の属するゾーンの ID を求める(結果はキャッシュ)
string Cloudflare::zone_id(const Config& config, const string& fqdn)
{
    if (auto it = zone_cache_.find(fqdn); it != zone_cache_.end())
        return it->second;
    for (const auto& zone : shochu::zone_candidates(fqdn)) {
        auto id = shochu::parse_cf_first_id(api(config, "GET", "/zones?name=" + zone, ""));
        if (id) {
            BOOST_LOG_TRIVIAL(debug) << "Cloudflare: zone " << zone << " = " << *id;
            zone_cache_[fqdn] = *id;
            return *id;
        }
    }
    throw runtime_error("Cloudflare: zone for " + fqdn + " not found (check the token permission)");
}

void Cloudflare::update_record(const Config& config, const string& fqdn, const string& ip)
{
    string zone = zone_id(config, fqdn);
    string base = "/zones/" + zone + "/dns_records";
    auto rec = shochu::parse_cf_record(api(config, "GET", base + "?type=A&name=" + fqdn, ""));
    if (!rec)
        throw runtime_error("Cloudflare: A record " + fqdn + " not found (create it first)");
    if (rec->content == ip) {
        BOOST_LOG_TRIVIAL(debug) << "Cloudflare: " << fqdn << " is up to date (" << ip << ")";
        return;
    }
    api(config, "PATCH", base + '/' + rec->id, shochu::cf_patch_body(ip));
    BOOST_LOG_TRIVIAL(info) << "Cloudflare: " << fqdn << " updated " << rec->content << " -> " << ip;
}

bool Cloudflare::update(const Config& config)
{
    if (!config.cf_enable())
        return true;
    auto records = config.cf_records();
    if (records.empty() || config.cf_api_token().empty()) {
        BOOST_LOG_TRIVIAL(warning) << "Cloudflare: ApiToken or Record0 is not configured";
        return true;
    }
    try {
        // 現在のグローバル IPv4 アドレス
        auto res = http_request("GET", config.cf_ip_check_url(), {}, "", config.http_timeout());
        if (res.status != 200)
            throw runtime_error("IP check returned HTTP " + to_string(res.status));
        auto ip = shochu::parse_trace_ip(res.body);
        if (!ip)
            throw runtime_error("IP check response has no valid IPv4 'ip=' line");

        string key = *ip;
        for (const auto& r : records)
            key += '|' + r;
        if (key == last_key_ && skipped_ < config.cf_recheck_cycles() - 1) {
            skipped_++;
            BOOST_LOG_TRIVIAL(debug) << "Cloudflare: IP is unchanged (" << *ip << "), skip";
            return true;
        }
        last_key_.clear();
        skipped_ = 0;

        bool all_ok = true;
        for (const auto& fqdn : records) {
            try {
                if (!shochu::is_valid_fqdn(fqdn))
                    throw runtime_error("invalid record name: " + fqdn);
                update_record(config, fqdn, *ip);
            }
            catch (const exception& x) {
                BOOST_LOG_TRIVIAL(error) << "Cloudflare: " << x.what();
                all_ok = false;
            }
        }
        if (all_ok)
            last_key_ = key;
        return all_ok;
    }
    catch (const exception& x) {
        BOOST_LOG_TRIVIAL(error) << "Cloudflare: " << x.what();
        return false;
    }
}

// end of <cloudflare.cpp>
