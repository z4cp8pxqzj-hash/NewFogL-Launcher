#pragma once

#include <string>

/**
 * TokenCrypto.h - 登录令牌的本地加密存取
 *
 * 职责：
 *   · 用 Windows DPAPI（当前用户作用域）加密令牌
 *   · 读取时兼容 "dpapi:xxx" 密文与旧版明文
 *
 * 不负责：
 *   · 令牌的网络校验（由 AuthService 负责）
 *   · 配置文件的读写（由 Config 负责）
 *
 * 依赖：
 *   · Windows Crypt32（DPAPI）
 *   · QtCore（Base64）
 *
 * 被依赖：
 *   · auth/AuthService - 读取令牌
 *   · 后续登录模块 - 写入令牌
 *
 * @author NewFogL
 * @since 1.0.0
 */
namespace NewFogL {

    class TokenCrypto {
    public:
        /**
         * 加密令牌
         *
         * @param plain 明文令牌
         * @return "dpapi:" + Base64 密文；失败返回空串
         */
        static std::string protect(const std::string& plain);

        /**
         * 解密令牌
         *
         * 不以 "dpapi:" 开头的输入按旧版明文直接返回，便于兼容已有配置
         *
         * @param stored 配置里存的字符串
         * @return 明文令牌；解密失败返回空串
         */
        static std::string unprotect(const std::string& stored);

        /**
         * 判断是否为 DPAPI 密文
         *
         * @param stored 配置里存的字符串
         * @return 是密文返回 true
         */
        static bool isProtected(const std::string& stored);
    };

}