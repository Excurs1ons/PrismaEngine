#include "Logger.h"
#include "IPlatformLogger.h"
#include "LogScope.h"
#include "pch.h"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#include <Windows.h>
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

void Logger::SetPlatformLogger(IPlatformLogger* platformLogger) {
    this->m_PlatformLogger = platformLogger;
}

CallStackOutput Logger::GetCallStackOutputForLevel(LogLevel level) {
    return CallStackOutput::None;
}

bool Logger::Initialize(const LogConfig& config) {
    if (m_Initialized) {
        return false;
    }
    m_Initialized = true;
    m_Config     = config;

    std::string logFilePath = m_Config.logFilePath;

    if (m_PlatformLogger != nullptr) {
        const char* platformLogDir = m_PlatformLogger->GetLogDirectoryPath();
        if (platformLogDir != nullptr) {
            std::filesystem::path originalPath(m_Config.logFilePath);
            std::filesystem::path platformPath(platformLogDir);
            platformPath /= originalPath.filename();
            logFilePath = platformPath.string();
        }
    }

    std::filesystem::path logPath(logFilePath);
    if (logPath.has_parent_path()) {
        std::filesystem::create_directories(logPath.parent_path());
    }

    if (static_cast<int>(m_Config.target) & static_cast<int>(LogTarget::File)) {
        m_FileStream.open(logFilePath, std::ios::app);
    }

#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (m_Config.enableColors) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode      = 0;
        if (GetConsoleMode(hConsole, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hConsole, mode);
        }
    }
#endif

    if (m_Config.asyncMode) {
        m_Running      = true;
        m_WorkerThread = std::make_unique<std::thread>(&Logger::ProcessQueue, this);
    }

    return true;
}

void Logger::Shutdown() {
    if (m_Config.asyncMode) {
        m_Running = false;
        m_QueueCondition.notify_one();
        if (m_WorkerThread && m_WorkerThread->joinable()) {
            m_WorkerThread->join();
        }
    }

    Flush();

    if (m_FileStream.is_open()) {
        m_FileStream.close();
    }
    m_Initialized = false;
}

Logger::~Logger() {
    if (m_Initialized) {
        Shutdown();
    }
}

void Logger::LogInternal(LogLevel level, const std::string& category, const std::string& message, SourceLocation loc) {
    if (level < m_Config.minLevel)
        return;

    LogEntry entry(level, message, category, loc);

    LogScope* currentScope = GetCurrentLogScope();
    if (currentScope) {
        currentScope->CacheLogEntry(entry);
    } else {
        if (m_Config.asyncMode) {
            EnqueueEntry(std::move(entry));
        } else {
            WriteEntry(entry);
        }
    }
}

std::string Logger::WStringToString(const std::wstring& wstr) {
    if (wstr.empty())
        return std::string();
#if defined(_WIN32)
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);   
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);      
    return strTo;
#else
    return std::filesystem::path(wstr).string();
#endif
}

void Logger::Flush() {
    if (m_FileStream.is_open()) {
        std::lock_guard<std::mutex> lock(m_WriteMutex);
        m_FileStream.flush();
    }
}

void Logger::EnqueueEntry(LogEntry&& entry) {
    std::unique_lock<std::mutex> lock(m_QueueMutex);
    if (m_LogQueue.size() >= m_Config.asyncQueueSize) {
        m_LogQueue.pop();
    }
    m_LogQueue.push(std::move(entry));
    m_QueueCondition.notify_one();
}

void Logger::ProcessQueue() {
    while (m_Running) {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_QueueCondition.wait(lock, [this] { return !m_LogQueue.empty() || !m_Running; });

        while (!m_LogQueue.empty()) {
            LogEntry entry = std::move(m_LogQueue.front());
            m_LogQueue.pop();
            lock.unlock();
            WriteEntry(entry);
            lock.lock();
        }
    }
}

void Logger::WriteEntry(const LogEntry& entry) {
    if (static_cast<int>(m_Config.target) & static_cast<int>(LogTarget::Console)) {
        std::string consoleMsg = FormatEntry(entry, m_Config.enableColors);
        WriteToConsole(consoleMsg, m_Config.enableColors);
    }

    if (static_cast<int>(m_Config.target) & static_cast<int>(LogTarget::File)) {
        std::string fileMsg = FormatEntry(entry, false);
        WriteToFile(fileMsg);
    }
}

void Logger::PushLogScope(LogScope* scope) {
    if (!scope) return;
    std::lock_guard<std::mutex> lock(m_scopeMutex);
    m_logScopes.push(scope);
}

void Logger::PopLogScope(LogScope* scope) {
    std::lock_guard<std::mutex> lock(m_scopeMutex);
    if (!m_logScopes.empty() && m_logScopes.top() == scope) {
        m_logScopes.pop();
    }
}

LogScope* Logger::GetCurrentLogScope() const {
    std::lock_guard<std::mutex> lock(m_scopeMutex);
    if (!m_logScopes.empty()) return m_logScopes.top();
    return nullptr;
}

std::vector<StackFrame> Logger::CaptureCallStack(int skipFrames, int maxFrames) {
    return {};
}

std::string Logger::FormatEntry(const LogEntry& entry, bool useColors) {
    std::ostringstream oss;
    if (useColors) {
        oss << ColorCode(GetLevelColor(entry.level));
    }
    if (m_Config.enableTimestamp) {
        oss << "[" << GetTimestamp(entry.timestamp) << "] ";
    }
    oss << "[" << GetLevelString(entry.level) << "] ";
    if (!entry.category.empty()) {
        oss << "[" << entry.category << "] ";
    }
    oss << entry.message;
    if (useColors) {
        oss << ColorCode(LogColor::Reset);
    }
    return oss.str();
}

std::string Logger::GetLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

LogColor Logger::GetLevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return LogColor::BrightBlack;
        case LogLevel::Debug:   return LogColor::Cyan;
        case LogLevel::Info:    return LogColor::Green;
        case LogLevel::Warning: return LogColor::Yellow;
        case LogLevel::Error:   return LogColor::Red;
        case LogLevel::Fatal:   return LogColor::BrightRed;
        default:                return LogColor::White;
    }
}

std::string Logger::ColorCode(LogColor color) {
    return std::format("\033[{}m", static_cast<int>(color));
}

std::string Logger::GetTimestamp(const std::chrono::system_clock::time_point& time) {
    auto timeT = std::chrono::system_clock::to_time_t(time);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void Logger::WriteToConsole(const std::string& message, bool useColors) {
    std::cout << message << std::endl;
}

void Logger::WriteToFile(const std::string& message) {
    if (m_FileStream.is_open()) {
        std::lock_guard<std::mutex> lock(m_WriteMutex);
        m_FileStream << message << std::endl;
    }
}

std::string Logger::FormatCallStack(const std::vector<StackFrame>& callStack) {
    return "";
}

void Logger::RotateLogFile() {}

} // namespace Prisma
