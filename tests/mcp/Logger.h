#pragma once
#include <string>
#include <iostream>

enum class LogLevel { Trace = 0, Debug, Info, Warn, Error, Fatal };

struct SourceLocation {};

class Logger {
public:
    static Logger& Get() { static Logger inst; return inst; }
    void LogInternal(LogLevel level, const std::string& tag, const std::string& msg, const SourceLocation& = {}) {
        const char* levels[] = {"TRACE","DEBUG","INFO","WARN","ERROR","FATAL"};
        std::cout << "[" << levels[static_cast<int>(level)] << "][" << tag << "] " << msg << std::endl;
    }

    template<typename... Args>
    void LogFormat(LogLevel level, const std::string& tag, const SourceLocation&, const std::string& msg, Args&&...) {
        LogInternal(level, tag, msg);
    }

    LogLevel GetMinLevel() const { return LogLevel::Trace; }
    void SetMinLevel(LogLevel) {}
    void Initialize() {}
    void Shutdown() {}
};

#define LOG_INFO(tag, ...)   Logger::Get().LogFormat(LogLevel::Info, tag, {}, __VA_ARGS__)
#define LOG_WARN(tag, ...)   Logger::Get().LogFormat(LogLevel::Warn, tag, {}, __VA_ARGS__)
#define LOG_ERROR(tag, ...)  Logger::Get().LogFormat(LogLevel::Error, tag, {}, __VA_ARGS__)
#define LOG_FATAL(tag, ...)  Logger::Get().LogFormat(LogLevel::Fatal, tag, {}, __VA_ARGS__)
