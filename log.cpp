// <log.cpp>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief log
//

#include "pch.h"
#pragma hdrstop

#include "log.h"

#include <winver.h>              // GetFileVersionInfoW

#include <boost/date_time/gregorian/gregorian.hpp> // boost::gregorian::greg_day
#include <boost/log/attributes/clock.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp> // add_common_attributes
#include <boost/log/utility/setup/console.hpp>           // add_console_log
#include <boost/log/utility/setup/file.hpp>              // add_file_log

#include <algorithm>            // std::clamp
#include <cwctype>              // std::towlower
#include <filesystem>           // std::filesystem::path
#include <iostream>             // std::clog
#include <string>               // std::string
#include <vector>               // std::vector

using namespace std;

namespace {

// exe の FileVersion を "a.b.c.d" で返す(取得できなければ空)
string file_version_string(const filesystem::path& path)
{
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    if (size == 0) return {};
    vector<char> data(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data())) return {};
    VS_FIXEDFILEINFO* info = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &len) || !info)
        return {};
    return to_string(HIWORD(info->dwFileVersionMS)) + '.' + to_string(LOWORD(info->dwFileVersionMS)) + '.'
        + to_string(HIWORD(info->dwFileVersionLS)) + '.' + to_string(LOWORD(info->dwFileVersionLS));
}

} // namespace

void InitLog(const Config& config)
{
    static bool inited = false;
    if (inited) return;
    inited = true;

    auto exe = Config::module_path();

    // ログディレクトリ: 相対パスは exe のあるディレクトリ起点。
    // exe が build\Debug|Release にあるときは 2 つ上のディレクトリを使う(開発用)
    filesystem::path log_dir = config.log_dir();
    if (log_dir.is_relative()) {
        auto base = exe.parent_path();
        auto leaf = base.filename().wstring();
        auto parent = base.parent_path().filename().wstring();
        auto lower = [](wstring s) { for (auto& c : s) c = static_cast<wchar_t>(towlower(c)); return s; };
        if ((lower(leaf) == L"debug" || lower(leaf) == L"release") && lower(parent) == L"build")
            base = base.parent_path().parent_path();
        log_dir = base / log_dir;
    }
    error_code ec;
    filesystem::create_directories(log_dir, ec);

    namespace logging = boost::log;
    namespace expr = boost::log::expressions;
    namespace keywords = boost::log::keywords;

    logging::add_common_attributes();   // TimeStamp(ローカル時刻) など
    auto format = expr::stream
        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
        << " [" << logging::trivial::severity << "] " << expr::smessage;

    // shochu_YYYYMM.log: 月初(1日 0:00)にファイル名が切り替わる
    logging::add_file_log(
        keywords::file_name = (log_dir / "shochu_%Y%m.log").string(),
        keywords::time_based_rotation =
            logging::sinks::file::rotation_at_time_point(boost::gregorian::greg_day(1), 0, 0, 0),
        keywords::open_mode = (ios::out | ios::app),
        keywords::auto_flush = true,
        keywords::format = format);
    if (GetConsoleWindow())             // -debug のときはコンソールにも出す
        logging::add_console_log(clog, keywords::format = format);

    // 深刻度フィルタ(0=fatal ... 5=trace)
    auto level = static_cast<logging::trivial::severity_level>(5 - clamp(config.log_level(), 0, 5));
    logging::core::get()->set_filter(logging::trivial::severity >= level);

    BOOST_LOG_TRIVIAL(info) << "Start " << exe.string() << " version " << file_version_string(exe);
}

// end of <log.cpp>
