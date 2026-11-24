// <log.h>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief log
//

#ifndef log_h
#define log_h

#include <boost/log/trivial.hpp> // BOOST_LOG_TRIVIAL

#include "config.h"             // Config

/// ログの準備(起動後初回のみ有効)
//
/// shochu_YYYYMM.log を LogDir に作る(月初にローテーション)。
/// コンソールがあるとき(-debug)はコンソールにも出力する
void InitLog(const Config& config);

#endif // log_h

// end of <log.h>
