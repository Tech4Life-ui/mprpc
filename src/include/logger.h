#pragma once
#include "lockqueue.h"
#include <string>

enum LogLevel
{
    INFO,
    ERROR
};

class Logger
{
public:
    static Logger &getInstance();
    void SetLogLevel(LogLevel level);
    void Log(std::string msg);

private:
    int m_loglevel;
    LockQueue<std::string> m_lcQue;
    Logger();
    Logger(const Logger &) = delete;
    Logger(Logger &&) = delete;
};

#define LOG_INFO(logmsgformat, ...)                     \
    do                                                  \
    {                                                   \
        Logger &logger = Logger::getInstance();         \
        logger.SetLogLevel(INFO);                       \
        char c[1024] = {0};                             \
        snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c);                                  \
    } while (0);

#define LOG_ERROR(logmsgformat, ...)                    \
    do                                                  \
    {                                                   \
        Logger &logger = Logger::getInstance();         \
        logger.SetLogLevel(ERROR);                      \
        char c[1024] = {0};                             \
        snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c);                                  \
    } while (0);