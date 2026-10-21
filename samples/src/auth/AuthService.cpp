#include "auth/AuthService.h"

#include <nlohmann/json.hpp>

#include "auth/TokenCrypto.h"
#include "core/Config.h"
#include "core/PathResolver.h"
#include "net/HttpClient.h"

/**
 * AuthService.cpp - 令牌与游戏所有权校验实现
 *
 * 说明：
 *   · profile 返回 401/403 视为令牌失效
 *   · entitlements 中命中 product_minecraft 或 game_minecraft 视为拥有游戏
 *   · 两次请求都走官方 api.minecraftservices.com，不经过任何镜像
 *
 * @author NewFogL
 * @since 1.0.0
 */
namespace NewFogL {

    namespace {
        constexpr const char* kProfileUrl      = "https://api.minecraftservices.com/minecraft/profile";
        constexpr const char* kEntitlementUrl  = "https://api.minecraftservices.com/entitlements/mcstore";
    }

    AuthResult AuthService::validateCurrentAccount() {
        AuthResult result;

        GlobalConfig cfg = Config::loadGlobal(PathResolver::getConfigPath());

        // 取默认正版账号
        const Account* account = nullptr;
        for (const auto& acc : cfg.accounts) {
            if (acc.type != "microsoft" || acc.username.empty()) continue;
            if (!account || acc.isDefault) account = &acc;
        }

        if (!account) {
            result.message = "没有可用的正版账号，请先登录。";
            return result;
        }

        const std::string token = TokenCrypto::unprotect(account->accessToken);
        if (token.empty()) {
            result.message = "账号缺少可用的访问令牌，请重新登录。";
            return result;
        }

        // 1. 验证令牌，顺便取玩家名与 UUID
        HttpResponse profile = HttpClient::get(kProfileUrl, token);
        if (profile.statusCode == 401 || profile.statusCode == 403) {
            result.message = "登录令牌已失效，请重新登录。";
            return result;
        }
        if (!profile.ok()) {
            result.message = "校验令牌失败（HTTP "
                           + std::to_string(profile.statusCode) + "）。"
                           + profile.error;
            return result;
        }

        try {
            nlohmann::json j = nlohmann::json::parse(profile.body);
            result.username = j.value("name", account->username);
            result.uuid     = j.value("id", account->uuid);
        } catch (const std::exception&) {
            result.message = "profile 返回内容无法解析。";
            return result;
        }
        result.tokenValid = true;

        // 2. 查询是否拥有 Minecraft
        HttpResponse entitlement = HttpClient::get(kEntitlementUrl, token);
        if (!entitlement.ok()) {
            result.message = "查询游戏所有权失败（HTTP "
                           + std::to_string(entitlement.statusCode) + "）。"
                           + entitlement.error;
            return result;
        }

        try {
            nlohmann::json j = nlohmann::json::parse(entitlement.body);
            if (j.contains("items") && j["items"].is_array()) {
                for (const auto& item : j["items"]) {
                    const std::string name = item.value("name", "");
                    if (name == "product_minecraft" || name == "game_minecraft") {
                        result.ownsGame = true;
                        break;
                    }
                }
            }
        } catch (const std::exception&) {
            result.message = "所有权返回内容无法解析。";
            return result;
        }

        if (!result.ownsGame) {
            result.message = "该账号未检测到 Minecraft 购买记录。";
            return result;
        }

        result.ok = true;
        result.message = "登录有效，账号拥有游戏。";
        return result;
    }

}