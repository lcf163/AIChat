#pragma once

#include <string>

namespace util {

/**
 * @brief 密码工具类，提供密码哈希和验证功能
 * 使用PBKDF2算法进行安全的密码存储和验证
 */
class PasswordUtil {
public:
    /**
     * @brief 生成安全的随机盐值
     * @param length 盐值长度，默认32字节
     * @return 生成的盐值（十六进制字符串）
     */
    static std::string generateSalt(size_t length = 32);

    /**
     * @brief 使用PBKDF2算法对密码进行哈希
     * @param password 明文密码
     * @param salt 盐值
     * @param iterations 迭代次数，默认10000次
     * @param keyLength 输出密钥长度，默认32字节
     * @return 哈希后的密码（十六进制字符串）
     */
    static std::string hashPassword(const std::string& password, 
                                   const std::string& salt,
                                   int iterations = 10000, 
                                   size_t keyLength = 32);

    /**
     * @brief 验证密码是否与哈希值匹配
     * @param password 明文密码
     * @param salt 盐值
     * @param hashedPassword 已存储的哈希密码
     * @param iterations 迭代次数
     * @param keyLength 密钥长度
     * @return 是否匹配
     */
    static bool verifyPassword(const std::string& password,
                              const std::string& salt,
                              const std::string& hashedPassword,
                              int iterations = 10000,
                              size_t keyLength = 32);
};

} // namespace util