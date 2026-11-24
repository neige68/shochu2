// <config.h>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief config
//

#ifndef config_h
#define config_h

#include <boost/property_tree/ptree.hpp> // boost::property_tree::ptree

#include <filesystem>           // std::filesystem::path
#include <string>               // std::string
#include <vector>               // std::vector

//------------------------------------------------------------
//
/// 設定(exe と同じディレクトリの shochu.ini)
//
/// 値型。周期ごとに load() し直すことで、サービスを再起動せずに ini の変更が反映される
//
class Config {

public:
    /// MyDNS のアカウント
    struct Account {
        std::string user;
        std::string password;
    };

    // *** static functions ***
    /// 実行中の exe のフルパス
    static std::filesystem::path module_path();
    /// ini ファイルを読み込む(ファイルが無い・読めない場合は std::runtime_error)
    static Config load();

    // *** functions: [SHOCHU] ***
    /// 正常時の間隔[秒]
    int normal_interval() const { return get_int("SHOCHU.NormalInterval", 900, 1); }
    /// 異常時の間隔[秒]
    int abnormal_interval() const { return get_int("SHOCHU.AbnormalInterval", 60, 1); }
    /// HTTP タイムアウト[秒]
    int http_timeout() const { return get_int("SHOCHU.HttpTimeOut", 10, 1); }
    /// ログレベル(0=fatal, 1=error, 2=warning, 3=info(default), 4=debug, 5=trace)
    int log_level() const { return get_int("SHOCHU.LogLevel", 3, 0); }
    /// ログディレクトリ(相対パスは exe のあるディレクトリが起点)
    std::string log_dir() const { return get_string("SHOCHU.LogDir", "log"); }

    // *** functions: [MYDNS] ***
    bool mydns_enable() const { return get_int("MYDNS.Enable", 0, 0) != 0; }
    std::string mydns_url() const
        { return get_string("MYDNS.Url", "https://ipv4.mydns.jp/login.html"); }
    /// User0/Password0, User1/Password1, ... (User が空なら終了, 抜け番は 9 まで許容)
    std::vector<Account> mydns_accounts() const;

    // *** functions: [CLOUDFLARE] ***
    bool cf_enable() const { return get_int("CLOUDFLARE.Enable", 0, 0) != 0; }
    std::string cf_api_token() const { return get_string("CLOUDFLARE.ApiToken"); }
    std::string cf_ip_check_url() const
        { return get_string("CLOUDFLARE.IpCheckUrl", "https://1.1.1.1/cdn-cgi/trace"); }
    /// IP が変わらなくても N 周期ごとにレコードを再確認する
    int cf_recheck_cycles() const { return get_int("CLOUDFLARE.RecheckCycles", 8, 1); }
    /// Record0, Record1, ... (空なら終了, 抜け番は 9 まで許容)
    std::vector<std::string> cf_records() const;

private:
    int get_int(const char* name, int default_value, int min_value) const;
    std::string get_string(const char* name, std::string default_value = std::string{}) const;

    boost::property_tree::ptree pt_;

};

#endif // config_h

// end of <config.h>
