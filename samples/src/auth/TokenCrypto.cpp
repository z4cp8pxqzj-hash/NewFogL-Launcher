#include "auth/TokenCrypto.h"

#include <QByteArray>

#include <windows.h>
#include <wincrypt.h>

/**
 * TokenCrypto.cpp - DPAPI 加解密实现
 *
 * 说明：
 *   · 密文格式固定为 dpapi:<Base64>，前缀用于区分旧版明文
 *   · 作用域为当前 Windows 用户，换用户或换机器无法解密，属于预期行为
 *
 * @author NewFogL
 * @since 1.0.0
 */
namespace NewFogL {

    namespace {
        constexpr const char* kPrefix = "dpapi:";
    }

    std::string TokenCrypto::protect(const std::string& plain) {
        if (plain.empty()) return "";

        DATA_BLOB in{};
        in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()));
        in.cbData = static_cast<DWORD>(plain.size());

        DATA_BLOB out{};
        if (!CryptProtectData(&in, L"NewFogL token", nullptr, nullptr,
                              nullptr, 0, &out)) {
            return "";
        }

        QByteArray blob(reinterpret_cast<const char*>(out.pbData),
                        static_cast<int>(out.cbData));
        LocalFree(out.pbData);

        return std::string(kPrefix) + blob.toBase64().toStdString();
    }

    std::string TokenCrypto::unprotect(const std::string& stored) {
        if (stored.empty()) return "";
        if (!isProtected(stored)) return stored;   // 兼容旧版明文

        QByteArray blob = QByteArray::fromBase64(
            QByteArray::fromStdString(stored.substr(std::string(kPrefix).size())));

        DATA_BLOB in{};
        in.pbData = reinterpret_cast<BYTE*>(blob.data());
        in.cbData = static_cast<DWORD>(blob.size());

        DATA_BLOB out{};
        if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr,
                                nullptr, 0, &out)) {
            return "";
        }

        std::string plain(reinterpret_cast<const char*>(out.pbData), out.cbData);
        LocalFree(out.pbData);
        return plain;
    }

    bool TokenCrypto::isProtected(const std::string& stored) {
        return stored.rfind(kPrefix, 0) == 0;
    }

}