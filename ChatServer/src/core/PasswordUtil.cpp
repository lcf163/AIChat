#include "core/PasswordUtil.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <vector>

namespace util {

std::string PasswordUtil::generateSalt(size_t length) {
    std::vector<unsigned char> salt(length);
    RAND_bytes(salt.data(), length);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < salt.size(); ++i) {
        ss << std::setw(2) << static_cast<unsigned>(salt[i]);
    }
    return ss.str();
}

std::string PasswordUtil::hashPassword(const std::string& password,
                                      const std::string& salt,
                                      int iterations,
                                      size_t keyLength) {
    std::vector<unsigned char> derivedKey(keyLength);
    
    // 将十六进制盐值转换为二进制
    std::vector<unsigned char> saltBytes(salt.length() / 2);
    for (size_t i = 0; i < salt.length(); i += 2) {
        std::string byteString = salt.substr(i, 2);
        saltBytes[i / 2] = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
    }
    
    PKCS5_PBKDF2_HMAC(
        password.c_str(), password.length(),
        saltBytes.data(), saltBytes.size(),
        iterations,
        EVP_sha256(),
        keyLength,
        derivedKey.data()
    );
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < derivedKey.size(); ++i) {
        ss << std::setw(2) << static_cast<unsigned>(derivedKey[i]);
    }
    return ss.str();
}

bool PasswordUtil::verifyPassword(const std::string& password,
                                 const std::string& salt,
                                 const std::string& hashedPassword,
                                 int iterations,
                                 size_t keyLength) {
    std::string computedHash = hashPassword(password, salt, iterations, keyLength);
    return computedHash == hashedPassword;
}

} // namespace util