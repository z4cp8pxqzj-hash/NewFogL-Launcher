#pragma once

#include <string>

/**
 * HttpClient.h - 基于 WinHTTP 的最小 HTTP 客户端
 *
 * 职责：
 *   · 提供同步 GET，返回状态码与响应体
 *   · 支持附加 Bearer 令牌
 *
 * 不负责：
 *   · 并发与异步（调用方自行决定在哪个线程执行）
 *   · 文件下载与断点续传（由 resource 层负责）
 *
 * 依赖：
 *   · Windows WinHTTP
 *
 * 被依赖：
 *   · auth/AuthService - 校验登录令牌与游戏所有权
 *
 * 说明：
 *   · 静态 Qt 只编了 Core/Gui/Widgets，没有 QtNetwork，因此走系统 WinHTTP
 *
 * @author NewFogL
 * @since 1.0.0
 */

namespace NewFogL {

    /**
     * HTTP 响应
     */
    struct HttpResponse {
        int         statusCode = 0;   // 0 表示请求根本没发出去
        std::string body;             // 响应体（UTF-8）
        std::string error;            // 失败原因

        /**
         * 是否为 2xx
         *
         * @return 成功返回 true
         */
        bool ok() const { return statusCode >= 200 && statusCode < 300; }
    };

    class HttpClient {
    public:
        /**
         * 发起同步 GET 请求
         *
         * @param url         完整 URL，必须带 https://
         * @param bearerToken 可选，非空时附加 Authorization: Bearer
         * @return 响应；网络层失败时 statusCode 为 0 且 error 非空
         */
        static HttpResponse get(const std::string& url,
                                const std::string& bearerToken = "");
    };

}