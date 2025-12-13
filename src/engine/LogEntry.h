#pragma once
#include <chrono>
#include <mutex>
#include <vector>
#include <string>

// 日志级别
enum class LogLevel { Trace = 0, Debug = 1, Info = 2, Warning = 3, Error = 4, Fatal = 5 };

// 调用堆栈输出行为
enum class CallStackOutput {
    None,        // 不输出调用堆栈
    CallerOnly,  // 仅输出调用者位置
    Full         // 输出完整调用堆栈
};

// 日志颜色（ANSI 颜色码）
enum class LogColor {
    Reset         = 0,
    Black         = 30,
    Red           = 31,
    Green         = 32,
    Yellow        = 33,
    Blue          = 34,
    Magenta       = 35,
    Cyan          = 36,
    White         = 37,
    BrightBlack   = 90,
    BrightRed     = 91,
    BrightGreen   = 92,
    BrightYellow  = 93,
    BrightBlue    = 94,
    BrightMagenta = 95,
    BrightCyan    = 96,
    BrightWhite   = 97
};

// 日志输出目标
enum class LogTarget { Console = 1 << 0, File = 1 << 1, Both = Console | File };

inline LogTarget operator|(LogTarget a, LogTarget b) {
    return static_cast<LogTarget>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(LogTarget a, LogTarget b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

// 源码位置信息
struct SourceLocation {
    const char* file;
    int line;
    const char* function;

    inline SourceLocation(const char* f = "", int l = 0, const char* func = "") : file(f), line(l), function(func) {}
};

// 调用堆栈帧信息
struct StackFrame {
    std::string file;
    int line = 0;
    std::string function;
    
    StackFrame() = default;
    StackFrame(const std::string& f, int l, const std::string& func)
        : file(f), line(l), function(func) {}
};

// 日志条目
struct LogEntry {
    LogLevel level;
    std::string message;
    std::string category;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id threadId;
    SourceLocation location;
    std::vector<StackFrame> callStack;  // 调用堆栈

    LogEntry(LogLevel lvl, std::string msg, std::string cat, SourceLocation loc = SourceLocation())
        : level(lvl), message(std::move(msg)), category(std::move(cat)), timestamp(std::chrono::system_clock::now()),
          threadId(std::this_thread::get_id()), location(loc) {}
};