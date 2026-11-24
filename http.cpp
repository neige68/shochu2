// <http.cpp>
//
// Project shochu
// Copyright (C) 2025-2026 neige68
//
/// \file
/// \brief http (WinHTTP wrapper)
//

#include "pch.h"
#pragma hdrstop

#include "http.h"

#include <winhttp.h>

#include <stdexcept>            // std::runtime_error

using namespace std;

namespace {

constexpr size_t MAX_BODY = 1024 * 1024;

wstring to_wide(const string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) throw runtime_error("invalid UTF-8 string");
    wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

[[noreturn]] void fail(const char* what)
{
    throw runtime_error(string(what) + " failed (error " + to_string(GetLastError()) + ")");
}

// WinHTTP ハンドルの RAII
class Handle {
public:
    explicit Handle(HINTERNET h = nullptr) : h_(h) {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { if (h_) WinHttpCloseHandle(h_); }
    operator HINTERNET() const { return h_; }
private:
    HINTERNET h_;
};

} // namespace

HttpResponse http_request(const string& method, const string& url, const vector<string>& headers,
                          const string& body, int timeout_sec)
{
    // URL を分解
    wstring wurl = to_wide(url);
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    uc.dwHostNameLength = static_cast<DWORD>(-1);
    uc.dwUrlPathLength = static_cast<DWORD>(-1);
    uc.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wurl.c_str(), static_cast<DWORD>(wurl.size()), 0, &uc))
        fail("WinHttpCrackUrl");
    if (uc.nScheme != INTERNET_SCHEME_HTTPS)
        throw runtime_error("only https:// URLs are allowed");
    wstring host(uc.lpszHostName, uc.dwHostNameLength);
    wstring path(uc.lpszUrlPath, uc.dwUrlPathLength);
    path.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);
    if (path.empty()) path = L"/";

    // WINHTTP_ACCESS_TYPE_DEFAULT_PROXY: サービス(LocalSystem)でもマシンのプロキシ設定に従う
    Handle session(WinHttpOpen(L"shochu/2.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) fail("WinHttpOpen");
    int ms = timeout_sec * 1000;
    if (!WinHttpSetTimeouts(session, ms, ms, ms, ms)) fail("WinHttpSetTimeouts");

    Handle connect(WinHttpConnect(session, host.c_str(), uc.nPort, 0));
    if (!connect) fail("WinHttpConnect");
    Handle request(WinHttpOpenRequest(connect, to_wide(method).c_str(), path.c_str(), nullptr,
                                      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                      WINHTTP_FLAG_SECURE));
    if (!request) fail("WinHttpOpenRequest");

    for (const auto& h : headers) {
        wstring wh = to_wide(h);
        if (!WinHttpAddRequestHeaders(request, wh.c_str(), static_cast<DWORD>(wh.size()),
                                      WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE))
            fail("WinHttpAddRequestHeaders");
    }

    DWORD body_len = static_cast<DWORD>(body.size());
    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()),
                            body_len, body_len, 0))
        fail("WinHttpSendRequest");
    if (!WinHttpReceiveResponse(request, nullptr)) fail("WinHttpReceiveResponse");

    HttpResponse result;
    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
                             WINHTTP_NO_HEADER_INDEX))
        fail("WinHttpQueryHeaders");
    result.status = static_cast<int>(status);

    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(request, &avail)) fail("WinHttpQueryDataAvailable");
        if (avail == 0) break;
        if (result.body.size() + avail > MAX_BODY)
            throw runtime_error("response body too large");
        size_t old = result.body.size();
        result.body.resize(old + avail);
        DWORD read = 0;
        if (!WinHttpReadData(request, result.body.data() + old, avail, &read)) fail("WinHttpReadData");
        result.body.resize(old + read);
        if (read == 0) break;
    }
    return result;
}

// end of <http.cpp>
