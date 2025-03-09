#include "mprpcconfig.h"
#include <iostream> // 标准输入输出

// -------------------------------- 核心方法：加载配置文件 --------------------------------
/**
 * @brief 加载并解析配置文件
 * @param config_file 配置文件路径
 *
 * 流程：
 * 1. 打开配置文件
 * 2. 逐行读取内容
 * 3. 去除注释和空白行
 * 4. 解析 key=value 格式
 * 5. 存储到配置字典中
 */
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    // 打开配置文件（只读模式）
    FILE *pf = fopen(config_file, "r");
    if (pf == nullptr)
    { // 文件打开失败处理
        std::cout << config_file << "is not exist" << std::endl;
        // 改进建议：增加 return 或 exit()
        return;
    }

    // 逐行读取文件内容
    while (!feof(pf))
    { // 注意：feof 在读取结束后才会返回 true
        char buf[512] = {0};
        fgets(buf, 512, pf); // 读取一行（最大 512 字节）

        std::string read_buf(buf);
        Trim(read_buf); // 去除首尾空格

        // 跳过注释行（# 开头）和空行
        if (read_buf[0] == '#' || read_buf.empty())
        {
            continue;
        }

        // 查找等号位置
        int idx = read_buf.find('=');
        if (idx == -1)
        { // 未找到等号视为无效行
            continue;
        }

        // 分割键值对
        std::string key;
        std::string value;
        key = read_buf.substr(0, idx); // 截取等号前部分为 key
        Trim(key);                     // 去除 key 的空白

        // 查找行尾（注意：Windows 换行符为 \r\n）
        int endidx = read_buf.find('\n', idx);
        value = read_buf.substr(idx + 1, endidx - idx - 1); // 截取等号后部分为 value
        Trim(value);                                        // 去除 value 的空白

        m_configMap.insert({key, value}); // 存入字典
    }

    fclose(pf); // 关闭文件（建议增加错误检查）
}

// -------------------------------- 配置项访问方法 --------------------------------
/**
 * @brief 根据键获取配置值
 * @param key 配置项名称
 * @return 配置值（不存在时返回空字符串）
 *
 */
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if (it == m_configMap.end())
        return "";     // 键不存在时返回空
    return it->second; // 返回对应的值
}

// -------------------------------- 辅助方法：去除首尾空格 --------------------------------
/**
 * @brief 去除字符串首尾空格
 * @param src_buf 待处理字符串
 *
 * 示例：
 * "  hello world  " → "hello world"
 */
void MprpcConfig::Trim(std::string &src_buf)
{
    // 去除头部空格
    int idx = src_buf.find_first_not_of(' '); // 查找第一个非空格字符位置
    if (idx != -1)
    {
        src_buf = src_buf.substr(idx, src_buf.size() - idx);
    }
    else
    {
        src_buf.clear(); // 全空格时清空字符串
    }

    // 去除尾部空格
    idx = src_buf.find_last_not_of(' ');
    if (idx != -1)
    {
        src_buf = src_buf.substr(0, idx + 1);
    }
    else
    {
        src_buf.clear(); // 处理全空格情况
    }
}