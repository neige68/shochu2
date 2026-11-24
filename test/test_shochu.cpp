// <test_shochu.cpp>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief parse.cpp の単体テスト(ネットワーク不要)
//

#define BOOST_TEST_MODULE test_shochu
#include <boost/test/included/unit_test.hpp>

#include "parse.h"

using namespace shochu;

BOOST_AUTO_TEST_CASE(test_base64)
{
    // RFC 4648 のテストベクタ
    BOOST_TEST(base64_encode("") == "");
    BOOST_TEST(base64_encode("f") == "Zg==");
    BOOST_TEST(base64_encode("fo") == "Zm8=");
    BOOST_TEST(base64_encode("foo") == "Zm9v");
    BOOST_TEST(base64_encode("foob") == "Zm9vYg==");
    BOOST_TEST(base64_encode("fooba") == "Zm9vYmE=");
    BOOST_TEST(base64_encode("foobar") == "Zm9vYmFy");
    BOOST_TEST(base64_encode("user:pass") == "dXNlcjpwYXNz");
}

BOOST_AUTO_TEST_CASE(test_is_ipv4)
{
    BOOST_TEST(is_ipv4("1.2.3.4"));
    BOOST_TEST(is_ipv4("0.0.0.0"));
    BOOST_TEST(is_ipv4("255.255.255.255"));
    BOOST_TEST(!is_ipv4(""));
    BOOST_TEST(!is_ipv4("1.2.3"));
    BOOST_TEST(!is_ipv4("1.2.3.4.5"));
    BOOST_TEST(!is_ipv4("256.1.1.1"));
    BOOST_TEST(!is_ipv4("01.2.3.4"));
    BOOST_TEST(!is_ipv4("1.2.3.4 "));
    BOOST_TEST(!is_ipv4("1.2.3."));
    BOOST_TEST(!is_ipv4(".1.2.3"));
    BOOST_TEST(!is_ipv4("1..2.3"));
    BOOST_TEST(!is_ipv4("a.b.c.d"));
    BOOST_TEST(!is_ipv4("2001:db8::1"));
}

BOOST_AUTO_TEST_CASE(test_is_valid_fqdn)
{
    BOOST_TEST(is_valid_fqdn("www.example.com"));
    BOOST_TEST(is_valid_fqdn("a-b_c.example.co.jp"));
    BOOST_TEST(is_valid_fqdn("*.example.com"));
    BOOST_TEST(!is_valid_fqdn(""));
    BOOST_TEST(!is_valid_fqdn(".example.com"));
    BOOST_TEST(!is_valid_fqdn("example.com."));
    BOOST_TEST(!is_valid_fqdn("a..example.com"));
    BOOST_TEST(!is_valid_fqdn("a b.example.com"));
    BOOST_TEST(!is_valid_fqdn("a.example.com/../x"));
    BOOST_TEST(!is_valid_fqdn("a.example.com?x=1"));
    BOOST_TEST(!is_valid_fqdn(std::string(254, 'a')));
}

BOOST_AUTO_TEST_CASE(test_parse_trace_ip)
{
    const std::string body =
        "fl=123f45\nh=1.1.1.1\nip=203.0.113.7\nts=1700000000.123\nvisit_scheme=https\n";
    BOOST_TEST(parse_trace_ip(body).value() == "203.0.113.7");
    // CRLF
    BOOST_TEST(parse_trace_ip("h=x\r\nip=198.51.100.2\r\n").value() == "198.51.100.2");
    // 無い / IPv4 でない
    BOOST_TEST(!parse_trace_ip("h=1.1.1.1\n").has_value());
    BOOST_TEST(!parse_trace_ip("ip=2001:db8::1\n").has_value());
    BOOST_TEST(!parse_trace_ip("").has_value());
}

BOOST_AUTO_TEST_CASE(test_zone_candidates)
{
    auto v = zone_candidates("a.b.example.com");
    BOOST_REQUIRE(v.size() == 3);
    BOOST_TEST(v[0] == "a.b.example.com");
    BOOST_TEST(v[1] == "b.example.com");
    BOOST_TEST(v[2] == "example.com");
    BOOST_TEST(zone_candidates("example.com").size() == 1);
    BOOST_TEST(zone_candidates("com").empty());
}

BOOST_AUTO_TEST_CASE(test_cf_parse)
{
    const std::string zones =
        R"({"result":[{"id":"zone123","name":"example.com"}],"success":true,"errors":[],"messages":[]})";
    BOOST_TEST(parse_cf_first_id(zones).value() == "zone123");

    const std::string empty = R"({"result":[],"success":true,"errors":[]})";
    BOOST_TEST(!parse_cf_first_id(empty).has_value());

    const std::string records =
        R"({"result":[{"id":"rec456","type":"A","name":"www.example.com","content":"203.0.113.7","proxied":false,"ttl":1}],"success":true,"errors":[]})";
    auto rec = parse_cf_record(records);
    BOOST_REQUIRE(rec.has_value());
    BOOST_TEST(rec->id == "rec456");
    BOOST_TEST(rec->content == "203.0.113.7");

    const std::string failure =
        R"({"success":false,"errors":[{"code":9109,"message":"Invalid access token"}],"result":null})";
    BOOST_TEST(!parse_cf_first_id(failure).has_value());
    BOOST_TEST(!parse_cf_record(failure).has_value());
    BOOST_TEST(cf_error_message(failure) == "9109: Invalid access token");

    BOOST_TEST(!parse_cf_record("not json").has_value());
    BOOST_TEST(cf_error_message("not json") == "invalid JSON");
    BOOST_TEST(cf_error_message(zones) == "");
}

BOOST_AUTO_TEST_CASE(test_cf_patch_body)
{
    BOOST_TEST(cf_patch_body("1.2.3.4") == R"({"content":"1.2.3.4"})");
}
