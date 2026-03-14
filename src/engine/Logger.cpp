#include "Logger.h"
#include "IPlatformLogger.h"
#include "LogScope.h"
#include "pch.h"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <ctime>

// 为了实现完全独立，Logger 直接包含必要的原生头文件，不经过 Platform
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace Prisma {

static Logger* s_loggerInstance = nullptr;

Logger& Logger::Get() {
    if (!s_loggerInstance) {
        static Logger localInstance;
        s_loggerInstance = &localInstance;
    }
    return *s_loggerInstance;
}

void Logger::SetInstance(Logger* instance) {
    s_loggerInstance = instance;
}

void Logger::SetMinLevel(LogLevel level) {
    m_Config.minLevel = level;
}

void Logger::SetTarget(LogTarget target) {
    m_Config.target = target;
}

void Logger::EnableColors(bool enable) {
    m_Config.enableColors = enable;
}

LogLevel Logger::GetMinLevel() const {
    return m_Config.minLevel;
}

bool Logger::IsInitialized() const {
    return m_Initialized;
}

bool Logger::Initialize(const LogConfig& config) {
    if (m_Initialized) return false;
    m_Config = config;
    m_Initialized = true;
    return true;
}

void Logger::Shutdown() {
    m_Initialized = false;
}

// 内部私有的独立颜色设置方法，不依赖 Platform
static void InternalSetConsoleColor(LogLevel level) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;
    WORD attr = 0;
    switch (level) {
        case LogLevel::Trace:   attr = FOREGROUND_INTENSITY; break;
        case LogLevel::Debug:   attr = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case LogLevel::Info:    attr = FOREGROUND_GREEN; break;
        case LogLevel::Warning: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case LogLevel::Error:   attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        case LogLevel::Fatal:   attr = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        default:                attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }
    SetConsoleTextAttribute(hConsole, attr);
#else
    switch (level) {
        case LogLevel::Trace:   std::cout << "\033[37m"; break;
        case LogLevel::Debug:   std::cout << "\033[36m"; break;
        case LogLevel::Info:    std::cout << "\033[32m"; break;
        case LogLevel::Warning: std::cout << "\033[33m"; break;
        case LogLevel::Error:   std::cout << "\033[31m"; break;
        case LogLevel::Fatal:   std::cout << "\033[41;37m"; break;
        default:                std::cout << "\033[0m"; break;
    }
#endif
}

static void InternalResetConsoleColor() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    std::cout << "\033[0m";
#endif
}

void Logger::LogInternal(LogLevel level, const char* tag, const std::string& message) {
    if (!m_Initialized || level < m_Config.minLevel) return;

    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif

    char timeStr[32];
    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);

    std::string levelStr;
    switch (level) {
        case LogLevel::Trace:   levelStr = "[TRACE]"; break;
        case LogLevel::Debug:   levelStr = "[DEBUG]"; break;
        case LogLevel::Info:    levelStr = "[INFO ]"; break;
        case LogLevel::Warning: levelStr = "[WARN ]"; break;
        case LogLevel::Error:   levelStr = "[ERROR]"; break;
        case LogLevel::Fatal:   levelStr = "[FATAL]"; break;
    }

    std::string fullMessage = "[" + std::string(timeStr) + "] " + levelStr + " [" + tag + "] " + message;

    // 控制台输出
    if (m_Config.target == LogTarget::Console || m_Config.target == LogTarget::Both) {
        if (m_Config.enableColors) InternalSetConsoleColor(level);
        std::cout << fullMessage << std::endl;
        if (m_Config.enableColors) InternalResetConsoleColor();
    }

    // 平台调试输出 (直接调用 API，不通过 Platform)
#ifdef _WIN32
    OutputDebugStringA(fullMessage.c_str());
    OutputDebugStringA("\n");
#endif
}

} // namespace Prisma
