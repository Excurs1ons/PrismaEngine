#pragma once
#include "LogEntry.h"
namespace Engine {

/**
 * @brief 平台日志接口
 *
 * 独立的轻量级接口，用于打破 Logger 与 Platform 之间的循环依赖。
 * Platform 实现此接口，Logger 通过此接口调用平台特定的日志功能。
 */
class IPlatformLogger {
public:
    virtual ~IPlatformLogger() = default;

    /**
     * @brief 将日志输出到平台特定的控制台
     * @param level 日志级别
     * @param tag 日志标签（类别）
     * @param message 日志消息
     *
     * 平台实现：
     * - Windows: 输出到 std::cout
     * - Android: 使用 __android_log_print 输出到 logcat
     * - Linux/SDL: 输出到 std::cout
     */
    virtual void LogToConsole(LogLevel level, const char* tag, const char* message) = 0;

    /**
     * @brief 获取日志文件存储目录路径
     * @return 日志目录路径（静态缓冲区，无需释放）
     *
     * 平台实现：
     * - Windows: %LOCALAPPDATA%\PrismaEngine\logs
     * - Android: /data/data/package.name/files/logs 或使用 SDL_GetPrefPath
     * - Linux: ~/.local/share/PrismaEngine/logs
     */
    virtual const char* GetLogDirectoryPath() const = 0;
};

} // namespace Engine
