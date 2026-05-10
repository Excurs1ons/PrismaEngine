#pragma once

// Ensure ENGINE_API is defined
#ifndef ENGINE_API
#if defined(_WIN32) || defined(_MSC_VER)
#if defined(ENGINE_EXPORTS) || defined(PRISMA_ENGINE_EXPORTS) || defined(PrismaEngine_EXPORTS)
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif
#else
#define ENGINE_API __attribute__((visibility("default")))
#endif
#endif

#include "LogEntry.h"
#include "LogScope.h"
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <format>
#include <fstream>
#include <mutex>
#include <queue>
#include <source_location>
#include <stack>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

namespace Prisma {
// 前置声明
class IPlatformLogger;

// 日志配置
struct LogConfig {
    LogLevel minLevel =
#if defined(DEBUG) || defined(_DEBUG)
        LogLevel::Debug;
#else
        LogLevel::Info;
#endif
    LogTarget target          = LogTarget::Both;
    bool enableColors         = true;
    bool enableTimestamp      = true;
    bool enableThreadId       = true;
    bool enableSourceLocation = true;
    bool enableCallStack      = false;
    bool asyncMode            = true;
    size_t asyncQueueSize     = 1024;
    std::string logFilePath   = "logs/engine.log";
    size_t maxFileSize        = 10 * 1024 * 1024;
    size_t maxFileCount       = 5;
};

class ENGINE_API Logger {
public:
    static Logger& Get();
    static void SetInstance(Logger* instance);

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&)                 = delete;
    Logger& operator=(Logger&&)      = delete;

    ~Logger();

    bool Initialize(const LogConfig& config = LogConfig());
    bool IsInitialized() const;
    void Shutdown();

    void SetPlatformLogger(IPlatformLogger* platformLogger);
    void LogInternal(LogLevel level, const std::string& category, const std::string& message, SourceLocation loc);

    void SetMinLevel(LogLevel level);
    void SetTarget(LogTarget target);
    void EnableColors(bool enable);

    void PushLogScope(LogScope* scope);
    void PopLogScope(LogScope* scope);
    LogScope* GetCurrentLogScope() const;

    static std::string WStringToString(const std::wstring& wstr);

    template <typename... Args>
    void Log(LogLevel level,
             std::string_view category,
             std::format_string<Args...> fmt,
             Args&&... args,
             std::source_location loc = std::source_location::current()) {
        if (level < GetMinLevel())
            return;
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        LogInternal(
            level, std::string(category), message, SourceLocation(loc.file_name(), loc.line(), loc.function_name()));
    }

    template <typename... Args>
    inline void LogFormat(LogLevel level,
                          const std::string& category,
                          SourceLocation loc,
                          std::format_string<Args...> fmt,
                          Args&&... args) {
        if (level < GetMinLevel())
            return;
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        LogInternal(level, category, message, loc);
    }

    void Flush();
    void WriteEntry(const LogEntry& entry);
    std::vector<StackFrame> CaptureCallStack(int skipFrames = 0, int maxFrames = 32);
    std::string FormatCallStack(const std::vector<StackFrame>& callStack);
    CallStackOutput GetCallStackOutputForLevel(LogLevel level);

    LogLevel GetMinLevel() const;

private:
    Logger() = default;

    bool m_Initialized                = false;
    IPlatformLogger* m_PlatformLogger = nullptr;
    void EnqueueEntry(LogEntry&& entry);
    void ProcessQueue();
    void RotateLogFile();

    std::string FormatEntry(const LogEntry& entry, bool useColors);
    std::string GetLevelString(LogLevel level);
    LogColor GetLevelColor(LogLevel level);
    std::string ColorCode(LogColor color);
    std::string GetTimestamp(const std::chrono::system_clock::time_point& time);

    void WriteToConsole(const std::string& message, bool useColors);
    void WriteToFile(const std::string& message);

    mutable std::stack<LogScope*> m_logScopes;
    mutable std::mutex m_scopeMutex;

    LogConfig m_Config;
    std::ofstream m_FileStream;
    size_t m_CurrentFileSize = 0;

    std::atomic<bool> m_Running{false};
    std::queue<LogEntry> m_LogQueue;
    std::mutex m_QueueMutex;
    std::condition_variable m_QueueCondition;
    std::unique_ptr<std::thread> m_WorkerThread;
    std::mutex m_WriteMutex;
};

}  // namespace Prisma

#define LOG_TRACE(category, fmt, ...)                                                                                  \
    ::Prisma::Logger::Get().LogFormat(::Prisma::LogLevel::Trace,                                                       \
                                      category,                                                                        \
                                      ::Prisma::SourceLocation(__FILE__, __LINE__, __func__),                          \
                                      fmt,                                                                             \
                                      ##__VA_ARGS__)

#define LOG_DEBUG(category, fmt, ...)                                                                                  \
    ::Prisma::Logger::Get().LogFormat(::Prisma::LogLevel::Debug,                                                       \
                                      category,                                                                        \
                                      ::Prisma::SourceLocation(__FILE__, __LINE__, __func__),                          \
                                      fmt,                                                                             \
                                      ##__VA_ARGS__)

#define LOG_INFO(category, fmt, ...)                                                                                   \
    ::Prisma::Logger::Get().LogFormat(::Prisma::LogLevel::Info,                                                        \
                                      category,                                                                        \
                                      ::Prisma::SourceLocation(__FILE__, __LINE__, __func__),                          \
                                      fmt,                                                                             \
                                      ##__VA_ARGS__)

#define LOG_WARNING(category, fmt, ...)                                                                                \
    ::Prisma::Logger::Get().LogFormat(::Prisma::LogLevel::Warning,                                                     \
                                      category,                                                                        \
                                      ::Prisma::SourceLocation(__FILE__, __LINE__, __func__),                          \
                                      fmt,                                                                             \
                                      ##__VA_ARGS__)

#define LOG_ERROR(category, fmt, ...)                                                                                  \
    ::Prisma::Logger::Get().LogFormat(::Prisma::LogLevel::Error,                                                       \
                                      category,                                                                        \
                                      ::Prisma::SourceLocation(__FILE__, __LINE__, __func__),                          \
                                      fmt,                                                                             \
                                      ##__VA_ARGS__)

#define LOG_FATAL(category, fmt, ...)                                                                                  \
    ::Prisma::Logger::Get().LogFormat(::Prisma::LogLevel::Fatal,                                                       \
                                      category,                                                                        \
                                      ::Prisma::SourceLocation(__FILE__, __LINE__, __func__),                          \
                                      fmt,                                                                             \
                                      ##__VA_ARGS__)

#define LOG_WARN LOG_WARNING
#define LOG_ERR LOG_ERROR
#define LOG_VERBOSE LOG_TRACE