#include "auth/MicrosoftAuth.h"

#include <chrono>
#include <filesystem>
#include <thread>

#include <nlohmann/json.hpp>

#include "auth/TokenCrypto.h"
#include "core/Config.h"
#include "core/PathResolver.h"
#include "net/HttpClient.h"

/**
 * MicrosoftAuth.cpp - 设备码登录实现
 *
 * 说明：
 *   · 应用类型为“个人 Microsoft 帐户”，端点固定用 consumers
 *   · Xbox Live / XSTS 的错误码会翻译成可读提示
 *   · 登录成功后 Minecraft 令牌与微软刷新令牌都经 DPAPI 加密
 *
 * @author NewFogL
 * @since 1.0.0
 */
namespace NewFogL {

    namespace {

        constexpr const char* kXboxAuthUrl  = "https://user.auth.xboxlive.com/user/authenticate";
        constexpr const char* kXstsUrl      = "https://xsts.auth.xboxlive.com/xsts/authorize";
        constexpr const char* kMcLoginUrl   = "https://api.minecraftservices.com/authentication/login_with_xbox";
        constexpr const char* kProfileUrl   = "https://api.minecraftservices.com/minecraft/profile";

        /**
         * 睡眠并支持取消
         *
         * @param seconds   总秒数
         * @param cancelled 取消回调
         * @return 被取消返回 true
         */
        bool sleepWithCancel(int seconds, const std::function<bool()>& cancelled) {
            for (int i = 0; i < seconds; ++i) {
                if (cancelled && cancelled()) return true;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            return cancelled && cancelled();
        }

        /**
         * 从响应体中取出错误描述
         *
         * @param body JSON 响应
         * @return 可读的错误信息
         */
        std::string errorOf(const nlohmann::json& body) {
            std::string err = body.value("error", "");
            std::string desc = body.value("error_description", "");
            if (err.empty()) return desc;
            if (desc.empty()) return err;
            return err + ": " + desc;
        }

    }

    bool MicrosoftAuth::startDeviceCode(DeviceCodeInfo& out, std::string& error) {
        const std::string body =
            std::string("client_id=") + kClientId +
            "&scope=XboxLive.signin%20offline_access";

        HttpResponse resp = HttpClient::postForm(kDeviceCodeUrl, body);
        if (!resp.ok()) {
            error = "申请设备码失败（HTTP " + std::to_string(resp.statusCode) + "）。" + resp.error;
            return false;
        }

        try {
            const nlohmann::json j = nlohmann::json::parse(resp.body);
            out.deviceCode      = j.value("device_code", "");
            out.userCode        = j.value("user_code", "");
            out.verificationUri = j.value("verification_uri", "");
            out.expiresIn       = j.value("expires_in", 900);
            out.interval        = j.value("interval", 5);
            out.message         = j.value("message", "");
        } catch (const std::exception&) {
            error = "设备码响应无法解析。";
            return false;
        }

        if (out.deviceCode.empty() || out.userCode.empty()) {
            error = "设备码响应缺少必要字段。";
            return false;
        }

        return true;
    }

    LoginResult MicrosoftAuth::pollAndLogin(const DeviceCodeInfo& info,
                                            std::function<bool()> cancelled) {
        LoginResult result;

        // 1. 轮询令牌端点
        std::string msaToken;
        std::string refreshToken;
        int interval = info.interval > 0 ? info.interval : 5;
        const int maxTries = info.expiresIn / (interval > 0 ? interval : 5);

        for (int tries = 0; tries < maxTries; ++tries) {
            if (cancelled && cancelled()) {
                result.message = "已取消登录。";
                return result;
            }

            const std::string body =
                std::string("grant_type=urn:ietf:params:oauth:grant-type:device_code")
                + "&client_id=" + kClientId
                + "&device_code=" + info.deviceCode;

            HttpResponse resp = HttpClient::postForm(kTokenUrl, body);

            if (resp.ok()) {
                try {
                    const nlohmann::json j = nlohmann::json::parse(resp.body);
                    msaToken     = j.value("access_token", "");
                    refreshToken = j.value("refresh_token", "");
                } catch (const std::exception&) {
                    result.message = "令牌响应无法解析。";
                    return result;
                }
                if (!msaToken.empty()) break;
            } else {
                try {
                    const nlohmann::json j = nlohmann::json::parse(resp.body);
                    const std::string err = j.value("error", async? "" : "");
                    (void)err;
                } catch (...) {}
            }

            if (sleepWithCancel(interval, cancelled)) {
                result.message = "已取消登录。";
                return result;
            }
        }

        if (msaToken.empty()) {
            result.message = "等待授权超时，请重新登录。";
            return result;
        }

        // 2. Xbox Live 认证
        nlohmann::json xblReq = {
            {"Properties", {
                {"AuthMethod", "RPS"},
                {"SiteName",   "user.auth.xboxlive.com"},
                {"RpsTicket",  "d=" + msaToken}
            }},
            {"RelyingParty", "http://auth.xboxlive.com"},
            {"TokenType",    "JWT"}
        };

        HttpResponse xblResp = HttpClient::postJson(kXboxAuthUrl, xblReq.dump());
        if (!xblResp.ok()) {
            result.message = "Xbox Live 认证失败（HTTP "
                           + std::to_string(xblResp.statusCode) + "）。";
            return result;
        }

        std::string xblToken;
        std::string uhs;
        try {
            const nlohmann::json j = nlohmann::json::parse(xblResp.body);
            xblToken = j.value("Token", "");
            uhs = j["DisplayClaims"]["xui"][0].value("uhs", "");
        } catch (const std::exception&) {
            result.message = "Xbox Live 响应无法解析。";
            return result;
        }

        // 3. XSTS 授权
        nlohmann::json xstsReq = {
            {"Properties", {
                {"SandboxId",  "RETAIL"},
                {"UserTokens", nlohmann::json::array({ xblToken })}
            }},
            {"RelyingParty", "rp://api.minecraftservices.com/"},
            {"TokenType",    "JWT"}
        };

        HttpResponse xstsResp = HttpClient::postJson(kXstsUrl, xstsReq.dump());
        if (xstsResp.statusCode == 401) {
            int xerr = 0;
            try {
                const nlohmann::json j = nlohmann::json::parse(xstsResp.body);
                xerr = j.value("XErr", 0);
            } catch (...) {}

            if (xerr == 2148916233) {
                result.message = "该账号没有 Minecraft: Java 版。";
            } else if (xerr == 2148916238) {
                result.message = "该账号所在国家/地区不支持 Xbox Live。";
            } else {
                result.message = "XSTS 授权被拒绝（XErr " + std::to_string(xerr) + "）。";
            }
            return result;
        }
        if (!xstsResp.ok()) {
            result.message = "XSTS 授权失败（HTTP "
                           + std::to_string(xstsResp.statusCode) + "）。";
            return result;
        }

        std::string xstsToken;
        try {
            const nlohmann::json j = nlohmann::json::parse(xstsResp.body);
            xstsToken = j.value("Token", "");
            uhs = j["DisplayClaims"]["xui"][0].value("uhs", uhs);
        } catch (const std::exception&) {
            result.message = "XSTS 响应无法解析。";
            return result;
        }

        // 4. Minecraft 登录
        nlohmann::json mcReq = {
            {"identityToken", "XBL3.0 x=" + uhs + ";" + xstsToken}
        };

        HttpResponse mcResp = HttpClient::postJson(kMcLoginUrl, mcReq.dump());
        if (!mcResp.ok()) {
            if (mcResp.statusCode == 401 || mcResp.body.find("Invalid app registration") != std::string::npos) {
                result.message = "应用尚未通过 Mojang 审核（Invalid app registration），请先提交审核。";
            } else {
                result.message = "Minecraft 登录失败（HTTP "
                               + std::to_string(mcResp.statusCode) + "）。";
            }
            return result;
        }

        std::string mcToken;
        try {
            const nlohmann::json j = nlohmann::json::parse(mcResp.body);
            mcToken = j.value("access_token", "");
        } catch (const std::exception&) {
            result.message = "Minecraft 登录响应无法解析。";
            return result;
        }

        // 5. 玩家档案
        HttpResponse profile = HttpClient::get(kProfileUrl, mcToken);
        if (!profile.ok()) {
            result.message = "获取玩家档案失败（HTTP "
                           + std::to_string(profile.statusCode) + "）。";
            return result;
        }

        try {
            const nlohmann::json j = nlohmann::json::parse(profile.body);
            result.username = j.value("name", "");
            result.uuid     = j.value("id", "");
        } catch (const std::exception&) {
            result.message = "玩家档案无法解析。";
            return result;
        }

        if (result.username.empty()) {
            result.message = "玩家档案缺少用户名。";
            return result;
        }

        result.accessToken  = mcToken;
        result.refreshToken = refreshToken;
        result.ok = true;
        result.message = "登录成功。";
        return result;
    }

    bool MicrosoftAuth::saveAccount(const LoginResult& result, std::string& error) {
        if (!result.ok) {
            error = "登录结果无效，未保存。";
            return false;
        }

        const std::string configPath = PathResolver::getConfigPath();
        const std::filesystem::path p(configPath);
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);

        GlobalConfig cfg = Config::loadGlobal(configPath);

        bool replaced = false;
        for (auto& acc : cfg.accounts) {
            if (acc.type != "microsoft") continue;
            if (acc.uuid == result.uuid || acc.username == result.username) {
                acc.username     = result.username;
                acc.uuid         = result.uuid;
                acc.accessToken  = TokenCrypto::protect(result.accessToken);
                acc.refreshToken = TokenCrypto::protect(result.refreshToken);
                acc.isDefault    = true;
                replaced = true;
            } else {
                acc.isDefault = false;
            }
        }

        if (!replaced) {
            for (auto& acc : cfg.accounts) acc.isDefault = false;

            Account acc;
            acc.id           = result.uuid;
            acc.type         = "microsoft";
            acc.username     = result.username;
            acc.uuid         = result.uuid;
            acc.accessToken  = TokenCrypto::protect(result.accessToken);
            acc.refreshToken = TokenCrypto::protect(result.refreshToken);
            acc.isDefault    = true;
            cfg.accounts.push_back(acc);
        }

        Config::saveGlobal(configPath, cfg);
        error.clear();
        return true;
    }

}