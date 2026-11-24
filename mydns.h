// <mydns.h>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief MyDNS.JP への IP アドレス通知
//

#ifndef mydns_h
#define mydns_h

#include "config.h"             // Config

/// [MYDNS] の全アカウントについて https://ipv4.mydns.jp/login.html に Basic 認証でアクセスする
//
/// MyDNS.JP はアクセス元のグローバル IP アドレスをそのアカウントのホストに登録する。
/// 無効(Enable=0)またはアカウント未設定のときは何もせず true を返す。
//
/// \return 全アカウントで成功したら true
bool MyDnsNotify(const Config& config);

#endif // mydns_h

// end of <mydns.h>
