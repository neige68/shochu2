// <config.cpp>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief config
//

#include "pch.h"
#pragma hdrstop

#include "config.h"

#include <boost/property_tree/ini_parser.hpp> // boost::property_tree::read_ini

#include <algorithm>            // std::max
#include <fstream>              // std::ifstream
#include <stdexcept>            // std::runtime_error

using namespace std;

//static
filesystem::path Config::module_path()
{
    vector<wchar_t> buf(MAX_PATH);
    for (;;) {
        DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (n == 0)
            throw runtime_error("GetModuleFileNameW failed");
        if (n < buf.size())
            return filesystem::path(wstring(buf.data(), n));
        buf.resize(buf.size() * 2);
    }
}

//static
Config Config::load()
{
    filesystem::path ini = module_path().parent_path() / "shochu.ini";
    ifstream ifs(ini);
    if (!ifs)
        throw runtime_error("cannot open " + ini.string());
    Config result;
    try {
        boost::property_tree::read_ini(ifs, result.pt_);
    }
    catch (const exception& x) {
        // パスワード等を含む行が例外メッセージに入らないよう、内容は出さない
        throw runtime_error("cannot parse " + ini.string() + ": " + x.what());
    }
    return result;
}

int Config::get_int(const char* name, int default_value, int min_value) const
{
    if (auto v = pt_.get_optional<int>(name))
        return max(*v, min_value);
    return default_value;
}

string Config::get_string(const char* name, string default_value) const
{
    if (auto v = pt_.get_optional<string>(name))
        return *v;
    return default_value;
}

vector<Config::Account> Config::mydns_accounts() const
{
    vector<Account> result;
    for (int i = 0, blank = 0; blank < 10; i++) {
        string user = get_string(("MYDNS.User" + to_string(i)).c_str());
        if (user.empty()) {
            blank++;
            continue;
        }
        blank = 0;
        result.push_back({user, get_string(("MYDNS.Password" + to_string(i)).c_str())});
    }
    return result;
}

vector<string> Config::cf_records() const
{
    vector<string> result;
    for (int i = 0, blank = 0; blank < 10; i++) {
        string rec = get_string(("CLOUDFLARE.Record" + to_string(i)).c_str());
        if (rec.empty()) {
            blank++;
            continue;
        }
        blank = 0;
        result.push_back(rec);
    }
    return result;
}

// end of <config.cpp>
