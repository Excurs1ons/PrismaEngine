#pragma once
#include "LogEntry.h"
namespace Prisma {

/* 平台日志接口 */
class IPlatformLogger {
public:
    virtual ~IPlatformLogger() = default;

    /* 将日志输出到平台特定的控制台 */
    virtual void LogToConsole(LogLevel level, const char* tag, const char* message) = 0;

    /* 获取日志文件存储目录路径 */
    virtual const char* GetLogDirectoryPath() const = 0;
};

} // namespace Engine
