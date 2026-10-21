#pragma once

#include <string>

/**
 * AuthService.h - 创建版本前的登录校验
 *
 * 职责：
 *   · 取出默认正版账号并解密其访问令牌
 *   · 调 profile 验证令牌有效，拿到玩家名与 UUID
 *   · 调 entitlements/mcstore 确认账号拥有 Minecraft
 *
 * 不负责：
 *   · 登录流程本身（令牌获取与刷新由后续账号模块负责）
 *   · 游戏文件下载（下载不带令牌，由 resource 层负责）
 *
 * 依赖：
 *   · Config / PathResolver - 读取账号
 *   · TokenCrypto - 解密令牌
 *   · HttpClient - 在线校验
 *
 * 被依赖：
 *   · VersionSelectPage - 新建实例前调用
 *
 * @author NewFogL
 * @since 1.0.0
 */
namespace NewFogL {

    /**
     * 校验结果
     */
    struct AuthResult {
        bool        ok         = false;  // 令牌有效且拥有游戏
        bool        tokenValid = false;  // profile 校验通过
        bool        ownsGame   = false;  // entitlements 命中
        std::string username;            // profile 返回的玩家名
        std::string uuid;                // profile 返回的玩家 UUID
        std::string message;             // 失败原因或成功说明
    };

    class AuthService {
    public:
        /**
         * 校验当前默认正版账号
         *
         * 同步执行，会发起两次 HTTPS 请求；调用方需自行处理界面等待状态
         *
         * @return 校验结果
         */
        static AuthResult validateCurrentAccount();
    };

}