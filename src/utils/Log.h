//
// Created by lance on 2022/12/9.
//

#ifndef MORROW_LOG_H
#define MORROW_LOG_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <type_traits>

#include "Singleton.h"

namespace morrow {
enum class LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

class Log : public Singleton<Log> {
    friend class Singleton<Log>;

public:
    void setLevel(LogLevel level);

    bool setFileOutput(const std::string& filename);

    template <typename... Args>
    void debug(const char* file, const char* function, int line, const std::string& format, Args... args) {
        log(LogLevel::LOG_DEBUG, file, function, line, format, args...);
    }

    template <typename... Args>
    void info(const char* file, const char* function, int line, const std::string& format, Args... args) {
        log(LogLevel::LOG_INFO, file, function, line, format, args...);
    }

    template <typename... Args>
    void warn(const char* file, const char* function, int line, const std::string& format, Args... args) {
        log(LogLevel::LOG_WARNING, file, function, line, format, args...);
    }

    template <typename... Args>
    void error(const char* file, const char* function, int line, const std::string& format, Args... args) {
        log(LogLevel::LOG_ERROR, file, function, line, format, args...);
    }

    template <typename... Args>
    void abort(bool condition, const char* file, const char* function, int line, Args&&... args) {
        if (condition) return;

        std::lock_guard<std::mutex> lock(mutex_);

        if constexpr (sizeof...(Args) == 0) {
            auto message = formatMessage(LogLevel::LOG_ERROR, file, function, line, "Abort triggered");
            std::cout << message;
            if (file_.is_open()) file_ << message;
        } else {
            auto message = formatMessage(LogLevel::LOG_ERROR, file, function, line, std::forward<Args>(args)...);
            std::cout << message;
            if (file_.is_open()) file_ << message;
        }

#if defined(_WIN32)
        __debugbreak();
#elif defined(__APPLE__)
        __builtin_trap();
#elif defined(__linux__)
        raise(SIGTRAP);
#endif
    }

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

private:
    Log() = default;

    ~Log();

    template <typename... Args>
    void log(LogLevel level, const char* file, const char* function, int line, const std::string& format, Args... args) {
        if (level < currentLevel_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        auto message = formatMessage(level, file, function, line, format, args...);

        std::cout << message << std::flush;

        if (file_.is_open()) {
            file_ << message << std::flush;
        }
    }

    std::string extractFileName(const char* filePath) {
        std::string path(filePath);
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            return path.substr(lastSlash + 1);
        }
        return path;
    }

    template <typename... Args>
    std::string formatMessage(LogLevel level, const char* file, const char* function, int line, const std::string& format, Args... args) {
        std::stringstream ss;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] ";

        switch (level) {
            case LogLevel::LOG_DEBUG:
                ss << "[DEBUG] ";
                break;
            case LogLevel::LOG_INFO:
                ss << "[INFO]  ";
                break;
            case LogLevel::LOG_WARNING:
                ss << "[WARN]  ";
                break;
            case LogLevel::LOG_ERROR:
                ss << "[ERROR] ";
                break;
        }

        // 添加文件名、函数名和行号
        ss << "[" << extractFileName(file) << ":" << line << "][" << function << "] ";

        formatImpl(ss, format, args...);
        ss << "\n";

        return ss.str();
    }

    template <typename T, typename... Args>
    void formatImpl(std::stringstream& ss,
                    const std::string& format,
                    T value,
                    Args... args) {
        size_t pos = format.find("{}");
        if (pos == std::string::npos) return;

        ss << format.substr(0, pos);
        if constexpr (std::is_enum_v<T>) {
            ss << static_cast<std::underlying_type_t<T>>(value);
        } else {
            ss << value;
        }
        formatImpl(ss, format.substr(pos + 2), args...);
    }

    void formatImpl(std::stringstream& ss, const std::string& format) {
        ss << format;
    }

    LogLevel currentLevel_ = LogLevel::LOG_INFO;
    std::ofstream file_;
    std::mutex mutex_;
};

#define LOG_D(...)  morrow::Log::getInstance().debug(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_I(...)  morrow::Log::getInstance().info(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_W(...)  morrow::Log::getInstance().warn(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_E(...)  morrow::Log::getInstance().error(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_A(...)  morrow::Log::getInstance().abort(__VA_ARGS__)

#define LOG_DEBUG(...)  LOG_D(__VA_ARGS__)
#define LOG_INFO(...)   LOG_I(__VA_ARGS__)
#define LOG_WARN(...)   LOG_W(__VA_ARGS__)
#define LOG_ERROR(...)  LOG_E(__VA_ARGS__)
} // namespace morrow

#endif //MORROW_LOG_H