// mprpcapplication.h 对应的实现文件
#include "./include/mprpcapplication.h"
#include <iostream>
#include <unistd.h> // 用于 getopt 函数
#include "mprpcapplication.h"

// 静态成员初始化
MprpcConfig MprpcApplication::m_config;

// 显示命令行参数帮助信息
void ShowArgsHelp()
{
    std::cout << "format: command -i <configfile>" << std::endl;
}

/**
 * @brief 初始化 RPC 应用程序配置
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 *
 * 功能流程：
 * 1. 检查参数数量
 * 2. 解析 -i 参数获取配置文件路径
 * 3. 加载配置文件内容
 * 4. 打印关键配置信息
 */
void MprpcApplication::Init(int argc, char **argv)
{
    // 参数检查：至少需要 2 个参数（程序名 + -i 参数）
    if (argc < 2)
    {
        ShowArgsHelp();
        exit(EXIT_FAILURE); // 直接退出程序
    }

    int c = 0;
    std::string config_file;
    // 使用 getopt 解析命令行参数（格式: -i configfile）
    while ((c = getopt(argc, argv, "i:")) != -1) // "i:" 表示 -i 后必须带参数
    {
        switch (c)
        {
        case 'i':                 // 匹配到 -i 参数
            config_file = optarg; // optarg 是参数值（配置文件路径）
            break;
        case '?': // 匹配到未定义的选项
            std::cout << "invalid args!" << std::endl;
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':': // 匹配到缺少参数的选项
            std::cout << "need <configfile>" << std::endl;
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default: // 其他情况
            break;
        }
    }

    // 加载配置文件到 m_config 对象
    m_config.LoadConfigFile(config_file.c_str());

    // 打印关键配置项（调试用）
    std::cout << "rpcserverip: " << m_config.Load("rpcserverip") << std::endl;
    std::cout << "rpcserverport: " << m_config.Load("rpcserverport") << std::endl;
    std::cout << "zookeeperip: " << m_config.Load("zookeeperip") << std::endl;
    std::cout << "zookeeperport: " << m_config.Load("zookeeperport") << std::endl;
}

/**
 * @brief 获取单例对象（饿汉式单例）
 * @return MprpcApplication& 单例引用
 *
 * 特点：
 * - static 局部变量保证线程安全（C++11 起）
 * - 程序启动时即创建实例
 */
MprpcApplication &MprpcApplication::getInstance()
{
    static MprpcApplication app; // 静态局部变量实现单例
    return app;
}

/**
 * @brief 获取配置对象引用
 * @return MprpcConfig& 配置对象引用
 *
 * 用途：
 * - 允许其他模块访问配置信息
 * - 例如：获取服务器IP、端口等信息
 */
MprpcConfig &MprpcApplication::GetConfig()
{
    return m_config;
}