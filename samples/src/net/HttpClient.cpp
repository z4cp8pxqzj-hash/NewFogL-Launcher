#include "net/HttpClient.h"

#include <windows.h>
#include <winhttp.h>

#include <string>

/**
 * HttpClient.cpp - WinHTTP 同步 GET 实现
 *
 * 说明：
 *   · URL 由 WinHttpCrackUrl 拆分，自动识别 https 并启用安全标志
 *   · 超时：连接 5 秒、发送 5 秒、接收 10 秒
 *
 * @author NewFogL
 * @since 1.0.0
 */
namespace NewFogL {

    namespace {

        std::wstring toWide(const std::string& s) {
            if (s.empty()) return std::wstring();
            int len = MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                          static_cast<int>(s.size()), nullptr, 0);
            std::wstring w(static_cast<size_t>(len), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                static_cast<int>(s.size()), &w[0], len);
            return w;
        }

        /**
         * 句柄守卫，析构时自动关闭
         */
        struct Handle {
            HINTERNET h = nullptr;
            ~Handle() { if (h) WinHttpCloseHandle(h); }
        };

    }

    HttpResponse HttpClient::get(const std::string& url,
                                 const std::string& bearerToken) {
        HttpResponse resp;

        const std::wstring wurl = toWide(url);

        URL_COMPONENTS uc{};
        uc.dwStructSize      = sizeof(uc);
        uc.dwSchemeLength    = static_cast<DWORD>(-1);
        uc.dwHostNameLength  = static_cast<DWORD>(-1);
        uc.dwUrlPathLength   = static_cast<DWORD>(-1);
        uc.dwExtraInfoLength = static_cast<DWORD>(-1);

        if (!WinHttpCrackUrl(wurl.c_str(), static_cast<DWORD>(wurl.size()), 0, &uc)) {
            resp.error = "URL 解析失败";
            return resp;
        }

        std::wstring host(uc.lpszHostName, uc.dwHostNameLength);
        std::wstring path(uc.lpszUrlPath, uc.dwUrlPathLength);
        if (uc.dwExtraInfoLength > 0) {
            path.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);
        }

        Handle session;
        session.h = WinHttpOpen(L"NewFogL/1.0",
                                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                WINHTTP_NO_PROXY_NAME,
                                WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session.h) {
            resp.error = "WinHttpOpen 失败";
            return resp;
        }
        WinHttpSetTimeouts(session.h, 5000, 5000, 10000, 15000);

        Handle conn;
        conn.h = WinHttpConnect(session.h, host.c_str(), uc.nPort, 0);
        if (!conn.h) {
            resp.error = "WinHttpConnect 失败";
            return resp;
        }

        const DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS)
                                ? WINHTTP_FLAG_SECURE : 0;

        Handle request;
        request.h = WinHttpOpenRequest(conn.h, L"GET", path.c_str(),
                                       nullptr, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!request.h) {
            resp.error = "WinHttpOpenRequest 失败";
            return resp;
        }

        WinHttpAddRequestHeaders(request.h, L"Accept: application/json",
                                 static_cast<DWORD>(-1),
                                 WINHTTP_ADDREQ_FLAG_ADD);

        if (!bearerToken.empty()) {
            const std::wstring header = L"Authorization: Bearer " + toWide(bearerToken);
            WinHttpAddRequestHeaders(request.h, header.c_str(),
                                     static_cast<DWORD>(-1),
                                     WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
        }

        if (!WinHttpSendRequest(request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            resp.error = "WinHttpSendRequest 失败";
            return resp;
        }

        if (!WinHttpReceiveResponse(request.h, nullptr)) {
            resp.error = "WinHttpReceiveResponse 失败";
            return resp;
        }

        DWORD status = 0;
        DWORD len = sizeof(status);
        WinHttpQueryHeaders(request.h,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &status, &len,
                            WINHTTP_NO_HEADER_INDEX);
        resp.statusCode = static_cast<int>(status);

        DWORD available = 0;
        do {
            available = 0;
            if (!WinHttpQueryDataAvailable(request.h, &available)) {
                resp.error = "WinHttpQueryDataAvailable 失败";
                break;
            }
            if (available == 0) break;

            std::string chunk(available, '\0');
            DWORD read = 0;
            if (!WinHttpReadData(request.h, &chunk[0], available, &read)) {
                resp.error = "WinHttpReadData 失败";
                break;
            }
            chunk.resize(read);
            resp.body += chunk;
        } while (available > 0);

        return resp;
    }

}