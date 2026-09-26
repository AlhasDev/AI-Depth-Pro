#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <iostream>

namespace AIDepthPro {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

class Logger {
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filePath);

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

private:
    Logger();
    ~Logger();

    LogLevel m_minLevel = LogLevel::Info;
    std::mutex m_mutex;
    std::ofstream m_fileStream;
};

#define LOG_DEBUG(msg) AIDepthPro::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) AIDepthPro::Logger::getInstance().info(msg)
#define LOG_WARN(msg) AIDepthPro::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) AIDepthPro::Logger::getInstance().error(msg)

} // namespace AIDepthPro
