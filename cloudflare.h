// <cloudflare.h>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief Cloudflare DNS の A レコード更新
//

#ifndef cloudflare_h
#define cloudflare_h

#include "config.h"             // Config

#include <map>                  // std::map
#include <string>               // std::string

//------------------------------------------------------------
//
/// Cloudflare DNS の A レコードを現在のグローバル IPv4 アドレスに書き換える
//
/// 1. IpCheckUrl(既定 https://1.1.1.1/cdn-cgi/trace)で現在の IPv4 アドレスを調べる
/// 2. Record0, Record1, ... の各 FQDN について、ゾーンと既存の A レコードを検索し、
///    content が異なるときだけ PATCH する(proxied, TTL は変更しない)。
///    A レコードが無いときは作らずエラーにする
//
/// IP アドレスとレコード一覧が前回と同じ間は API を呼ばない(RecheckCycles 周期ごとに再確認する)
//
class Cloudflare {

public:
    /// 1 周期分の処理。無効(Enable=0)またはレコード未設定のときは何もせず true
    //
    /// \return すべて成功したら true
    bool update(const Config& config);

private:
    std::string zone_id(const Config& config, const std::string& fqdn);
    std::string api(const Config& config, const std::string& method, const std::string& path,
                    const std::string& body);
    void update_record(const Config& config, const std::string& fqdn, const std::string& ip);

    std::string last_key_;      ///< 前回すべて成功したときの "IP|レコード一覧"
    int skipped_ = 0;           ///< last_key_ が同じため API 呼び出しを省略した回数
    std::map<std::string, std::string> zone_cache_; ///< FQDN -> ゾーン ID

};

#endif // cloudflare_h

// end of <cloudflare.h>
