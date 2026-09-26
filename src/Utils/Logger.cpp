#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace AIDepthPro {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() = default;

Logger::~Logger() {
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

void Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    m_fileStream.open(filePath, std::ios::out | std::ios::app);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < m_minLevel) return;

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif

    const char* levelStr = "INFO";
    switch (level) {
        case LogLevel::Debug: levelStr = "DEBUG"; break;
        case LogLevel::Info: levelStr = "INFO"; break;
        case LogLevel::Warning: levelStr = "WARN"; break;
        case LogLevel::Error: levelStr = "ERROR"; break;
    }

    std::ostringstream ss;
    ss << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
       << "." << std::setfill('0') << std::setw(3) << ms.count()
       << "] [" << levelStr << "] " << message << "\n";

    std::string formatted = ss.str();

    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << formatted;
    if (m_fileStream.is_open()) {
        m_fileStream << formatted;
        m_fileStream.flush();
    }
}

void Logger::debug(const std::string& message) { log(LogLevel::Debug, message); }
void Logger::info(const std::string& message) { log(LogLevel::Info, message); }
void Logger::warning(const std::string& message) { log(LogLevel::Warning, message); }
void Logger::error(const std::string& message) { log(LogLevel::Error, message); }

} // namespace AIDepthPro
