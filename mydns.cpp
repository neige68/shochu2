// <mydns.cpp>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief MyDNS.JP への IP アドレス通知
//

#include "pch.h"
#pragma hdrstop

#include "mydns.h"

#include "http.h"               // http_request
#include "log.h"                // BOOST_LOG_TRIVIAL
#include "parse.h"              // shochu::base64_encode

#include <exception>            // std::exception
#include <string>               // std::string

using namespace std;

bool MyDnsNotify(const Config& config)
{
    if (!config.mydns_enable())
        return true;
    auto accounts = config.mydns_accounts();
    if (accounts.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "MyDNS: no account is configured (User0/Password0)";
        return true;
    }
    bool all_ok = true;
    for (size_t i = 0; i < accounts.size(); i++) {
        // ユーザ名・パスワードはログに出さない(番号のみ)
        try {
            DWORD t0 = GetTickCount();
            auto cred = shochu::base64_encode(accounts[i].user + ':' + accounts[i].password);
            auto res = http_request("GET", config.mydns_url(), {"Authorization: Basic " + cred}, "",
                                    config.http_timeout());
            DWORD ms = GetTickCount() - t0;
            if (res.status == 200) {
                BOOST_LOG_TRIVIAL(info) << "MyDNS notify succeeded (account " << i << "). " << ms << "ms.";
            }
            else {
                BOOST_LOG_TRIVIAL(error) << "MyDNS notify failed (account " << i << "): HTTP " << res.status;
                all_ok = false;
            }
        }
        catch (const exception& x) {
            BOOST_LOG_TRIVIAL(error) << "MyDNS notify failed (account " << i << "): " << x.what();
            all_ok = false;
        }
    }
    return all_ok;
}

// end of <mydns.cpp>
