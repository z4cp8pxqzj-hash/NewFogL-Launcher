#pragma once

#include <functional>
#include <string>

/**
 * MicrosoftAuth.h - 微软账号设备码登录
 *
 * 职责：
 *   · 申请设备码，供用户在 microsoft.com/link 输入
 *   · 轮询令牌端点，拿到微软访问令牌
 *   · 串联 Xbox Live → XSTS → Minecraft Services → 玩家档案
 *   · 把账号与加密令牌写回全局配置
 *
 * 不负责：
 *   · 登录界面（由 LoginDialog 负责）
 *   · 令牌的后续校验与刷新（由 AuthService 负责）
 *
 * 依赖：
 *   · HttpClient - HTTPS 请求
 *   · TokenCrypto - 令牌加密
 *   · Config / PathResolver - 账号持久化
 *   · nlohmann-json - 解析响应
 *
 * 被依赖：
 *   · ui/LoginDialog - 登录窗口
 *
 * @author NewFogL
 * @since 1.0.0
 */
namespace NewFogL {

    /**
     * 设备码信息
     */
    struct DeviceCodeInfo {
        std::string deviceCode;       // 轮询用的设备码
        std::string userCode;         // 展示给用户的短码
        std::string verificationUri;  // 用户输入短码的网址
        int         expiresIn = 900;  // 有效期（秒）
        int         interval  = 5;    // 建议轮询间隔（秒）
        std::string message;          // 微软给的提示文案
    };

    /**
     * 登录结果
     */
    struct LoginResult {
        bool        ok = false;
        std::string username;      // 玩家名
        std::string uuid;          // 玩家 UUID
        std::string accessToken;   // Minecraft 访问令牌
        std::string refreshToken;  // 微软刷新令牌
        std::string message;       // 失败原因或成功说明
    };

    class MicrosoftAuth {
    public:
        // Azure 应用（个人 Microsoft 帐户），审核通过前 Minecraft 登录会报 Invalid app registration
        static constexpr const char* kClientId = "125912e4-6354-438c-824a-85aaf2d8ebd0";

        static constexpr const char* kDeviceCodeUrl =
            "https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode";
        static constexpr const char* kTokenUrl =
            "https://login.microsoftonline.com/consumers/oauth2/v2.0/token";
        static constexpr const char* kScope = "XboxLive.signin offline_access";

        /**
         * 申请设备码
         *
         * @param out   申请结果
         * @param error 失败原因
         * @return 成功返回 true
         */
        static bool startDeviceCode(DeviceCodeInfo& out, std::string& error);

        /**
         * 轮询令牌并完成整条登录链
         *
         * 该调用会阻塞，应放在工作线程执行
         *
         * @param info      设备码信息
         * @param cancelled 取消回调，返回 true 时立即中止
         * @return 登录结果
         */
        static LoginResult pollAndLogin(const DeviceCodeInfo& info,
                                        std::function<bool()> cancelled);

        /**
         * 把登录结果写入全局配置
         *
         * 令牌经 DPAPI 加密后存储
         *
         * @param result 登录结果
         * @param error  失败原因
         * @return 成功返回 true
         */
        static bool saveAccount(const LoginResult& result, std::string& error);
    };

}