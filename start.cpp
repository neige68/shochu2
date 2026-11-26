// <start.cpp>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief service start
//

#include "pch.h"
#pragma hdrstop

#include "cloudflare.h"         // Cloudflare
#include "config.h"             // Config
#include "log.h"                // InitLog
#include "mydns.h"              // MyDnsNotify
#include "service.h"            // ServiceStart, ServiceStop

#include <algorithm>            // std::min
#include <exception>            // std::exception
#include <stdexcept>            // std::runtime_error
#include <string>               // std::string

using namespace std;

namespace {

/// 停止要求イベント(ServiceStop が SetEvent する)
HANDLE g_stop_event = nullptr;

/// スリープ復帰通知(ServiceResume が SetEvent する。自動リセット)
HANDLE g_resume_event = nullptr;

/// スリープ復帰後、ネットワークが戻るのを待ってから 1 周期実行するまでの待ち[ms]
constexpr DWORD RESUME_DELAY = 10 * 1000;

/// ini が読めないときの再試行間隔[ms]
constexpr DWORD CONFIG_ERROR_INTERVAL = 60 * 1000;

/// 1 周期分の処理。次の周期までの待ち時間[ms]を返す
DWORD Cycle(Cloudflare& cloudflare)
{
    Config config;
    try {
        config = Config::load();    // 周期ごとに読み直す(ini の変更は再起動なしで反映)
    }
    catch (const exception& x) {
        BOOST_LOG_TRIVIAL(error) << x.what();
        return CONFIG_ERROR_INTERVAL;
    }
    if (!config.mydns_enable() && !config.cf_enable()) {
        BOOST_LOG_TRIVIAL(warning) << "Neither [MYDNS] nor [CLOUDFLARE] is enabled in shochu.ini "
                                      "(set Enable=1; the old [POP3] format is not supported). See shochu.ini.sample.";
        return CONFIG_ERROR_INTERVAL;
    }
    // 片方が失敗してももう片方は実行する
    bool mydns_ok = true;
    bool cloudflare_ok = true;
    try { mydns_ok = MyDnsNotify(config); }
    catch (const exception& x) { BOOST_LOG_TRIVIAL(error) << "MyDNS: " << x.what(); mydns_ok = false; }
    try { cloudflare_ok = cloudflare.update(config); }
    catch (const exception& x) { BOOST_LOG_TRIVIAL(error) << "Cloudflare: " << x.what(); cloudflare_ok = false; }
    int sec = (mydns_ok && cloudflare_ok) ? config.normal_interval() : config.abnormal_interval();
    return static_cast<DWORD>(min(sec, 24 * 60 * 60)) * 1000;
}

} // namespace

/// 処理を行うサービスの実際のコード。ServiceStop でイベントが立つと戻る
void ServiceStart(DWORD dwArgc [[maybe_unused]], LPTSTR* lpszArgv [[maybe_unused]])
{
    try {
        g_stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        g_resume_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!g_stop_event || !g_resume_event)
            throw runtime_error("CreateEvent failed");

        // ini が読めなくてもログは出したい。読めなければ既定値でログを開始する
        Config config;
        string config_error;
        try { config = Config::load(); }
        catch (const exception& x) { config_error = x.what(); }
        InitLog(config);
        if (!config_error.empty())
            BOOST_LOG_TRIVIAL(error) << config_error;

        if (!ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0))
            throw runtime_error("ReportStatusToSCMgr failed.");
        BOOST_LOG_TRIVIAL(info) << "Running";

        Cloudflare cloudflare;
        DWORD wait_ms = 0;          // 起動直後はすぐに実行
        const HANDLE events[] = {g_stop_event, g_resume_event};
        for (;;) {
            // 待ち時間にはスリープ中の時間が含まれないので、復帰通知でも起きる
            const DWORD r = WaitForMultipleObjects(2, events, FALSE, wait_ms);
            if (r == WAIT_OBJECT_0)
                break;              // 停止要求
            if (r == WAIT_OBJECT_0 + 1) {
                BOOST_LOG_TRIVIAL(info) << "Resumed from sleep; next cycle in " << RESUME_DELAY / 1000 << " sec";
                wait_ms = RESUME_DELAY;
                continue;
            }
            if (r != WAIT_TIMEOUT)
                throw runtime_error("WaitForMultipleObjects failed");
            wait_ms = Cycle(cloudflare);
        }
    }
    catch (const exception& x) {
        try { BOOST_LOG_TRIVIAL(fatal) << x.what(); }
        catch (...) {}
        AddToMessageLog(TEXT("ServiceStart failed."));
    }
    try { BOOST_LOG_TRIVIAL(info) << "End"; }
    catch (...) {}
}

void ServiceStop()
{
    if (g_stop_event)
        SetEvent(g_stop_event);
}

void ServiceResume()
{
    if (g_resume_event)
        SetEvent(g_resume_event);
}

// end of <start.cpp>
