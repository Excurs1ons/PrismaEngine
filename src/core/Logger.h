#pragma once
#include "Singleton.h"
#include "LogScope.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <queue>
#include <source_location>
#include <stack>
#include <string>
#include <thread>
#include <type_traits>

// 日志配置
struct LogConfig {
    LogLevel minLevel         = 
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
    bool asyncMode            = true;
    size_t asyncQueueSize     = 1024;
    std::string logFilePath   = "logs/engine.log";
    size_t maxFileSize        = 10 * 1024 * 1024;  // 10MB
    size_t maxFileCount       = 5;                 // 日志文件轮转
};

// 编译时格式化检查
template<typename T>
concept Formattable = requires(T && t) {
    std::format("{}", std::forward<T>(t));
};

// 检查是否是字符串字面量或const char*类型
template<typename T>
concept StringType = std::is_same_v<std::decay_t<T>, const char*> || 
                     std::is_same_v<std::decay_t<T>, char*> ||
                     std::is_convertible_v<T, std::string_view> ||
                     requires(T && t) {
                         std::format("{}", std::forward<T>(t));
                     };

template<typename... Args>
constexpr bool CheckFormattable() {
    return (StringType<Args> && ...);
}

class Logger:public Singleton<Logger>
{
	//友元声明：允许Singleton访问私有构造函数
    friend class Singleton<Logger>;
public:
    ~Logger();

    // 初始化和关闭
    bool Initialize(const LogConfig& config = LogConfig());
    bool IsInitialized() const;
    void Shutdown();

    void LogInternal(LogLevel level, const std::string& category, const std::string& message, SourceLocation loc);

    // 设置配置
    void SetMinLevel(LogLevel level) { config_.minLevel = level; }
    void SetTarget(LogTarget target) { config_.target = target; }
    void EnableColors(bool enable) { config_.enableColors = enable; }

    // 日志作用域相关方法
    void PushLogScope(LogScope* scope);
    void PopLogScope(LogScope* scope);
    LogScope* GetCurrentLogScope() const;

    // 辅助函数：宽字符串转 UTF-8
    static std::string WStringToString(const std::wstring& wstr);

    // 日志输出（带源码位置）
    template<typename... Args>
    void Log(LogLevel level, std::string_view category, std::format_string<Args...> fmt, Args&&... args,
        std::source_location loc = std::source_location::current()) {

        if (level < config_.minLevel) return;

        std::string message = std::format(fmt, std::forward<Args>(args)...);
        LogInternal(level, std::string(category), message, 
            SourceLocation(loc.file_name(), loc.line(), loc.function_name()));
    }

    // 支持宽字符的日志输出
    template<typename... Args>
    void Log(LogLevel level, std::string_view category, std::wformat_string<Args...> fmt, Args&&... args,
        std::source_location loc = std::source_location::current()) {

        if (level < config_.minLevel) return;

        std::wstring wmessage = std::format(fmt, std::forward<Args>(args)...);
        LogInternal(level, std::string(category), WStringToString(wmessage), 
            SourceLocation(loc.file_name(), loc.line(), loc.function_name()));
    }

    // 格式化日志输出
    template<typename... Args>
    inline void LogFormat(LogLevel level, const std::string& category,
        SourceLocation loc, std::format_string<Args...> fmt, Args&&... args) {


        // 添加友好的编译错误信息
        static_assert(CheckFormattable<Args...>(),
            "One or more log arguments are not formattable.");

        if (level < config_.minLevel) return;
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        LogInternal(level, category, message, loc);
    }

    // 支持宽字符的格式化日志输出
    template<typename... Args>
    inline void LogFormat(LogLevel level, const std::string& category,
        SourceLocation loc, std::wformat_string<Args...> fmt, Args&&... args) {

        if (level < config_.minLevel) return;
        std::wstring wmessage = std::format(fmt, std::forward<Args>(args)...);
        LogInternal(level, category, WStringToString(wmessage), loc);
    }
    // 快捷方法
    template<typename... Args>
    void Trace(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Trace, category, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void Trace(std::string_view category, std::wformat_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Trace, category, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Debug(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Debug, category, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void Debug(std::string_view category, std::wformat_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Debug, category, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Info(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Info, category, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void Info(std::string_view category, std::wformat_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Info, category, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Warning(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Warning, category, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void Warning(std::string_view category, std::wformat_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Warning, category, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Error(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Error, category, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void Error(std::string_view category, std::wformat_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Error, category, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Fatal(std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Fatal, category, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void Fatal(std::string_view category, std::wformat_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Fatal, category, fmt, std::forward<Args>(args)...);
    }

    // 刷新日志
    void Flush();

    // 写入日志条目（公开供LogScope使用）
    void WriteEntry(const LogEntry& entry);

private:
    bool initialized_ = false;
    // 内部方法
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

    // 日志作用域相关成员
    mutable std::stack<LogScope*> m_logScopes;
    mutable std::mutex m_scopeMutex;

    // 成员变量
    LogConfig config_;
    std::ofstream fileStream_;
    size_t currentFileSize_ = 0;

    // 异步相关
    std::atomic<bool> running_{ false };
    std::queue<LogEntry> logQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::unique_ptr<std::thread> workerThread_;

    std::mutex writeMutex_;  // 保护文件写入
};

// 宏定义 - 自动捕获文件名、行号和函数名
#define LOG_TRACE(category, fmt, ...) \
    ::Logger::GetInstance().LogFormat(::LogLevel::Trace, category, \
        ::SourceLocation(__FILE__, __LINE__, __func__), fmt, ##__VA_ARGS__)

#define LOG_DEBUG(category, fmt, ...) \
    ::Logger::GetInstance().LogFormat(::LogLevel::Debug, category, \
        ::SourceLocation(__FILE__, __LINE__, __func__), fmt, ##__VA_ARGS__)

#define LOG_INFO(category, fmt, ...) \
    ::Logger::GetInstance().LogFormat(::LogLevel::Info, category, \
        ::SourceLocation(__FILE__, __LINE__, __func__), fmt, ##__VA_ARGS__)

#define LOG_WARNING(category, fmt, ...) \
    ::Logger::GetInstance().LogFormat(::LogLevel::Warning, category, \
        ::SourceLocation(__FILE__, __LINE__, __func__), fmt, ##__VA_ARGS__)

#define LOG_ERROR(category, fmt, ...) \
    ::Logger::GetInstance().LogFormat(::LogLevel::Error, category, \
        ::SourceLocation(__FILE__, __LINE__, __func__), fmt, ##__VA_ARGS__)

#define LOG_FATAL(category, fmt, ...) \
    ::Logger::GetInstance().LogFormat(::LogLevel::Fatal, category, \
        ::SourceLocation(__FILE__, __LINE__, __func__), fmt, ##__VA_ARGS__)
// 方式2: 简单字符串版本（不需要格式化）
#define LOG_INFO_SIMPLE(category, message) \
    ::Logger::GetInstance().LogInternal(::LogLevel::Info, category, \
        message, GET_SOURCE_LOCATION())

#define LOG_WARN LOG_WARNING
#define LOG_ERR LOG_ERROR
#define LOG_VERBOSE LOG_TRACE