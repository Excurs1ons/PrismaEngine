#include "Environment.h"
#include "../engine/Logger.h"
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <VersionHelpers.h>
#elif defined(__linux__)
    #include <cstdlib>
    #include <unistd.h>
    #include <fcntl.h>
#elif defined(__ANDROID__)
    #include <android/log.h>
#endif

namespace PrismaEngine {

EnvironmentType Environment::DetectEnvironment() {
    // 首先检查是否有显示支持
    if (HasDisplaySupport()) {
        // 检查是否在终端中运行（如果有显示但也在终端中，可能是用户主动选择）
        if (IsRunningInTerminal()) {
            return EnvironmentType::Desktop;
        }
        return EnvironmentType::Desktop;
    }

    // 无显示支持
    if (IsRunningInTerminal()) {
        return EnvironmentType::Terminal;
    }

    return EnvironmentType::Headless;
}

bool Environment::HasDisplaySupport() {
#if defined(_WIN32) || defined(_WIN64)
    return DetectDisplayWindows();
#elif defined(__linux__)
    return DetectDisplayLinux();
#elif defined(__ANDROID__)
    return DetectDisplayAndroid();
#else
    return false;
#endif
}

bool Environment::IsRunningInTerminal() {
    return !IsRedirectedOutput();
}

std::string Environment::GetEnvironmentDescription() {
    EnvironmentType env = DetectEnvironment();

    switch (env) {
        case EnvironmentType::Desktop:
            return "Desktop (GUI available)";
        case EnvironmentType::Terminal:
            return "Terminal (no GUI)";
        case EnvironmentType::Headless:
            return "Headless (no terminal or GUI)";
        case EnvironmentType::Unknown:
        default:
            return "Unknown";
    }
}

// ========== 平台特定实现 ==========

bool Environment::DetectDisplayLinux() {
    // 检查 DISPLAY 环境变量（X11）
    const char* display = std::getenv("DISPLAY");
    if (display && display[0] != '\0') {
        LOG_DEBUG("Environment", "检测到 X11 显示: {}", display);
        return true;
    }

    // 检查 WAYLAND_DISPLAY (Wayland)
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    if (wayland && wayland[0] != '\0') {
        LOG_DEBUG("Environment", "检测到 Wayland 显示: {}", wayland);
        return true;
    }

    // 检查是否在 SSH 会话中（可能没有 X11 转发）
    const char* ssh_client = std::getenv("SSH_CLIENT");
    const char* ssh_connection = std::getenv("SSH_CONNECTION");
    if ((ssh_client && ssh_client[0] != '\0') || (ssh_connection && ssh_connection[0] != '\0')) {
        // 检查是否有 X11 转发
        const char* display = std::getenv("DISPLAY");
        if (!display || display[0] == '\0') {
            LOG_DEBUG("Environment", "SSH 会话，但无 X11 转发");
            return false;
        }
    }

    LOG_DEBUG("Environment", "未检测到显示系统");
    return false;
}

bool Environment::DetectDisplayWindows() {
    // Windows 总是有图形界面（除非是 Windows Server Core）
    // 检查是否是核心版本
    #if defined(_WIN32) || defined(_WIN64)
        // Windows 桌面版本总是有图形界面
        // Windows Server 可能有也可能没有
        #ifdef VER_SUITE_SERVER
            // 服务器版本，尝试创建一个窗口来测试
            HWND hwnd = GetDesktopWindow();
            return hwnd != NULL;
        #else
            return true;
        #endif
    #else
        return false;
    #endif
}

bool Environment::DetectDisplayAndroid() {
    // Android 总是有显示（在设备上）
    return true;
}

bool Environment::IsRedirectedOutput() {
    // 检查标准输出是否被重定向
#if defined(_WIN32) || defined(_WIN64)
    // Windows: 检查是否是控制台程序
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut == INVALID_HANDLE_VALUE) {
        return true; // 无标准输出句柄，可能被重定向
    }

    DWORD mode;
    if (!GetConsoleMode(hStdOut, &mode)) {
        // 不是控制台，输出被重定向
        return true;
    }

    return false;
#elif defined(__linux__)
    // Linux/Unix: 检查文件描述符
    return !isatty(STDOUT_FILENO);
#else
    // 其他平台：假设没有被重定向
    return false;
#endif
}

} // namespace PrismaEngine
