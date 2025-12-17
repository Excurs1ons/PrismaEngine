#include "Logger.h"
#include "LogScope.h"
#include "pch.h"
#include <filesystem>
#include <iostream>
#if defined (_WIN32)
#include <windows.h>
#if defined(_MSC_VER)
#include <intrin.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif
#endif
bool Logger::IsInitialized() const {
    return initialized_;
}

CallStackOutput Logger::GetCallStackOutputForLevel(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
    case LogLevel::Debug:
#ifdef _DEBUG
        return CallStackOutput::Full;
#else
        return CallStackOutput::None;     // Trace/Debug级别不捕获调用堆栈（性能优化）
#endif
    case LogLevel::Info:
        return CallStackOutput::None;     // Info级别不输出调用堆栈
    case LogLevel::Warning:                  // Warning级别仅输出调用点
    case LogLevel::Error:
        return CallStackOutput::CallerOnly;  // Error级别仅输出调用点
    case LogLevel::Fatal:
        return CallStackOutput::Full;     // Fatal级别输出完整调用堆栈
    default:
        return CallStackOutput::None;
    }
}

/// @brief 初始化日志系统
bool Logger::Initialize(const LogConfig& config)
{
    if (initialized_) {
        LogInternal(LogLevel::Warning,
                    "Engine",
                    "日志系统已初始化，无法重复初始化",
                    SourceLocation(__FILE__, __LINE__, __FUNCTION__));
        return false;
    }
    initialized_ = true;
    config_ = config;

    // 创建日志目录
    std::filesystem::path logPath(config_.logFilePath);
    if (logPath.has_parent_path()) {
        std::filesystem::create_directories(logPath.parent_path());
    }

    // 打开日志文件
    if (config_.target & LogTarget::File) {
        fileStream_.open(config_.logFilePath, std::ios::app);
        if (!fileStream_.is_open()) {
            std::cerr << "Failed to open log file: " << config_.logFilePath << std::endl;
        }
        else {
            if (std::filesystem::exists(config_.logFilePath)) {
                currentFileSize_ = std::filesystem::file_size(config_.logFilePath);
            }
        }
    }
#if defined (_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // Windows 控制台启用 ANSI 颜色支持
    if (config_.enableColors) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hConsole, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, mode);
    }
#endif
    // 启动异步工作线程
    if (config_.asyncMode) {
        running_ = true;
        workerThread_ = std::make_unique<std::thread>(&Logger::ProcessQueue, this);
    }

    LogInternal(LogLevel::Info, "Engine", "日志系统初始化完成", SourceLocation(__FILE__, __LINE__, __FUNCTION__));
    return true;
}
/// @brief 关闭日志系统
void Logger::Shutdown() {
    if (config_.asyncMode) {
        running_ = false;
        queueCondition_.notify_one();
        if (workerThread_ && workerThread_->joinable()) {
            workerThread_->join();
        }
    }

    Flush();

    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

Logger::~Logger() {
    Shutdown();
}

void Logger::LogInternal(LogLevel level, const std::string& category,
    const std::string& message, SourceLocation loc) {
    if (level < config_.minLevel) return;

    LogEntry entry(level, message, category, loc);

    // 只有启用调用堆栈时才捕获
    if (config_.enableCallStack) {
        // 根据日志级别决定是否捕获调用堆栈
        CallStackOutput stackOutput = GetCallStackOutputForLevel(level);
        if (stackOutput != CallStackOutput::None) {
            // 捕获调用堆栈（跳过2帧：CaptureCallStack 和 LogInternal）
            entry.callStack = CaptureCallStack(2);
        }
    }

    // 检查是否有活跃的日志作用域
    LogScope* currentScope = GetCurrentLogScope();
    if (currentScope) {
        currentScope->CacheLogEntry(entry);
    }
    else {
        if (config_.asyncMode) {
            EnqueueEntry(std::move(entry));
        }
        else {
            WriteEntry(entry);
        }
    }
}

std::string Logger::WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
#if defined(_WIN32)
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#else
    return std::filesystem::path(wstr).string();
#endif
}

/// @brief 刷新日志输出
void Logger::Flush() {
    if (fileStream_.is_open()) {
        std::lock_guard<std::mutex> lock(writeMutex_);
        fileStream_.flush();
    }
}

void Logger::EnqueueEntry(LogEntry&& entry) {
    std::unique_lock<std::mutex> lock(queueMutex_);

    // 如果队列满了，等待或丢弃旧日志
    if (logQueue_.size() >= config_.asyncQueueSize) {
        logQueue_.pop();  // 丢弃最旧的日志
    }

    logQueue_.push(std::move(entry));
    queueCondition_.notify_one();
}

void Logger::ProcessQueue() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        queueCondition_.wait(lock, [this] {
            return !logQueue_.empty() || !running_;
            });

        while (!logQueue_.empty()) {
            LogEntry entry = std::move(logQueue_.front());
            logQueue_.pop();
            lock.unlock();

            WriteEntry(entry);

            lock.lock();
        }
    }

    // 处理剩余的日志
    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!logQueue_.empty()) {
        WriteEntry(logQueue_.front());
        logQueue_.pop();
    }
}

void Logger::WriteEntry(const LogEntry& entry) {
    if (config_.target & LogTarget::Console) {
        std::string consoleMsg = FormatEntry(entry, config_.enableColors);
        WriteToConsole(consoleMsg, config_.enableColors);
    }

    if (config_.target & LogTarget::File) {
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
    if (!m_logScopes.empty()) {
        return m_logScopes.top();
    }
    return nullptr;
}

std::vector<StackFrame> Logger::CaptureCallStack(int skipFrames, int maxFrames) {
    std::vector<StackFrame> frames;

#if defined(_WIN32) && defined(_MSC_VER)
    // Windows平台使用StackWalk64
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    CONTEXT context;
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);
    
    SymInitialize(process, NULL, TRUE);
    
    DWORD image;
#ifdef _M_IX86
    image = IMAGE_FILE_MACHINE_I386;
    STACKFRAME64 stackframe;
    ZeroMemory(&stackframe, sizeof(STACKFRAME64));
    stackframe.AddrPC.Offset = context.Eip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Ebp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Esp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    image = IMAGE_FILE_MACHINE_AMD64;
    STACKFRAME64 stackframe;
    ZeroMemory(&stackframe, sizeof(STACKFRAME64));
    stackframe.AddrPC.Offset = context.Rip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Rsp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Rsp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
    image = IMAGE_FILE_MACHINE_IA64;
    STACKFRAME64 stackframe;
    ZeroMemory(&stackframe, sizeof(STACKFRAME64));
    stackframe.AddrPC.Offset = context.StIIP;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.IntSp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrBStore.Offset = context.RsBSP;
    stackframe.AddrBStore.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.IntSp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#endif
    
    // 跳过指定的帧数（包括当前函数）
    for (int i = 0; i < skipFrames + 1; i++) {
        if (!StackWalk64(
            image, process, thread,
            &stackframe, &context, NULL,
            SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            break;
        }
    }
    
    // 收集调用堆栈帧
    for (int frame = 0; frame < maxFrames; frame++) {
        if (!StackWalk64(
            image, process, thread,
            &stackframe, &context, NULL,
            SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            break;
        }
        
        // 获取符号信息
        DWORD64 displacement = 0;
        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        
        std::string functionName = "Unknown";
        if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol)) {
            functionName = symbol->Name;
        }
        
        // 获取行号信息
        DWORD lineDisplacement = 0;
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        std::string fileName = "Unknown";
        DWORD lineNumber = 0;
        
        if (SymGetLineFromAddr64(process, stackframe.AddrPC.Offset, &lineDisplacement, &line)) {
            fileName = line.FileName;
            lineNumber = line.LineNumber;
        }
        
        frames.emplace_back(fileName, lineNumber, functionName);
        
        // 如果到达了main函数，停止展开堆栈
        if (functionName == "main" || functionName == "WinMain") {
            break;
        }
    }
    
    SymCleanup(process);
#endif
    
    return frames;
}

/// 格式化日志条目
std::string Logger::FormatEntry(const LogEntry& entry, bool useColors) {
	//ostringstream像c#的StringBuilder一样，用于高效地构建字符串
    std::ostringstream oss;

    // 颜色开始
    if (useColors) {
        oss << ColorCode(GetLevelColor(entry.level));
    }

    // 时间戳
    if (config_.enableTimestamp) {
        oss << "[" << GetTimestamp(entry.timestamp) << "] ";
    }

    // 日志级别
    oss << "[" << GetLevelString(entry.level) << "] ";

    // 分类
    if (!entry.category.empty()) {
        oss << "[" << entry.category << "] ";
    }

    // 线程ID
    if (config_.enableThreadId) {
        oss << "[Thread:" << entry.threadId << "] ";
    }

    // 消息
    oss << entry.message;

    // 源码位置和调用堆栈
    CallStackOutput stackOutput = GetCallStackOutputForLevel(entry.level);
    if (config_.enableSourceLocation && stackOutput != CallStackOutput::None) {
        // 只显示文件名，不显示完整路径
        std::filesystem::path filePath(entry.location.file);
        oss << " (" << filePath.filename().string()
            << ":" << entry.location.line << ")";
            
        // 显示调用堆栈
        if (stackOutput == CallStackOutput::Full && !entry.callStack.empty()) {
            oss << "\nCall Stack:\n" << FormatCallStack(entry.callStack);
        }
    }

    // 颜色重置
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
/// 获取颜色代码字符串
std::string Logger::ColorCode(LogColor color) {
    return std::format("\033[{}m", static_cast<int>(color));
}

std::string Logger::GetTimestamp(const std::chrono::system_clock::time_point& time) {
    auto timeT = std::chrono::system_clock::to_time_t(time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch()) % 1000;

    std::tm tm;
    //如果是Windows平台，使用localtime_s函数，否则使用localtime_r函数
    //localtime_s是线程安全的，而localtime_r不是线程安全的
//#ifdef _WIN32
    localtime_s(&tm, &timeT);
//#else
//#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Logger::WriteToConsole(const std::string& message, bool useColors) {
    std::cout << message << std::endl;
}

void Logger::WriteToFile(const std::string& message) {
    if (!fileStream_.is_open()) return;

    std::lock_guard<std::mutex> lock(writeMutex_);

    fileStream_ << message << std::endl;
    currentFileSize_ += message.length() + 1;

    // 检查是否需要轮转
    if (currentFileSize_ >= config_.maxFileSize) {
        RotateLogFile();
    }
}

std::string Logger::FormatCallStack(const std::vector<StackFrame>& callStack) {
    std::ostringstream oss;
    
    for (size_t i = 0; i < callStack.size(); ++i) {
        const auto& frame = callStack[i];
        // 只显示文件名，不显示完整路径
        std::filesystem::path filePath(frame.file);
        oss << "  " << i << ". " << frame.function 
            << " (" << filePath.filename().string() << ":" << frame.line << ")\n";
    }
    
    return oss.str();
}
/// 轮转日志文件，保存旧文件并重命名
void Logger::RotateLogFile() {
    fileStream_.close();

    std::filesystem::path logPath(config_.logFilePath);
    std::string baseName = logPath.stem().string();
    std::string extension = logPath.extension().string();
    std::filesystem::path directory = logPath.parent_path();

    // 轮转现有文件
    for (int i = static_cast<int>(config_.maxFileCount) - 1; i > 0; --i) {
        std::string oldFile = std::format("{}/{}_{}{}",
            directory.string(), baseName, i, extension);
        std::string newFile = std::format("{}/{}_{}{}",
            directory.string(), baseName, i + 1, extension);

        if (std::filesystem::exists(oldFile)) {
            std::filesystem::rename(oldFile, newFile);
        }
    }

    // 重命名当前文件
    std::string backupFile = std::format("{}/{}_{}{}",
        directory.string(), baseName, 1, extension);
    std::filesystem::rename(config_.logFilePath, backupFile);

    // 创建新文件
    fileStream_.open(config_.logFilePath, std::ios::out);
    currentFileSize_ = 0;
}