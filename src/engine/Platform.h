#pragma once

#include "Export.h"
#include "IPlatformLogger.h"
#include "input/InputManager.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

#include "core/Event.h"

namespace Prisma {

class Platform {
public:
    static bool s_initialized;
    static bool s_shouldClose;
    static WindowHandle s_currentWindow;

    using EventCallback = std::function<void(Event&)>;
    static EventCallback s_eventCallback;

    // ------------------------------------------------------------
    // 平台生命周期管理
    // ------------------------------------------------------------
    ENGINE_API static bool Initialize();
    ENGINE_API static void Shutdown();
    ENGINE_API static bool IsInitialized();

    // ------------------------------------------------------------
    // 调试与控制台
    // ------------------------------------------------------------
    ENGINE_API static void DebugPrint(const char* message);
    ENGINE_API static void SetConsoleColor(LogLevel level);
    ENGINE_API static void ResetConsoleColor();
    ENGINE_API static uint32_t GetProcessId();
    ENGINE_API static std::tm GetLocalTime(std::time_t time);
    ENGINE_API static void ShowMessageBox(const std::string& title, const std::string& message);
    ENGINE_API static bool HasDisplaySupport();
    ENGINE_API static bool IsRunningInTerminal();
    ENGINE_API static std::string GetEnvironmentVariable(const std::string& name);
    ENGINE_API static void SetEnvironmentVariable(const std::string& name, const std::string& value);

    // ------------------------------------------------------------
    // Vulkan 支持
    // ------------------------------------------------------------
    ENGINE_API static std::vector<const char*> GetRequiredVulkanInstanceExtensions();
    ENGINE_API static bool CreateVulkanSurface(void* instance, WindowHandle window, void** outSurface);

    // ------------------------------------------------------------
    // 窗口管理
    // ------------------------------------------------------------
    ENGINE_API static WindowHandle CreateWindow(const WindowProps& desc);
    ENGINE_API static void DestroyWindow(WindowHandle window);
    ENGINE_API static void GetWindowSize(WindowHandle window, int& outW, int& outH);
    ENGINE_API static void SetWindowTitle(WindowHandle window, const char* title);
    ENGINE_API static void PumpEvents();
    ENGINE_API static void Update() { PumpEvents(); }
    ENGINE_API static bool ShouldClose(WindowHandle window);
    ENGINE_API static void SetShouldClose(WindowHandle window, bool shouldClose);
    ENGINE_API static WindowHandle GetCurrentWindow();


    // ------------------------------------------------------------
    // 时间管理
    // ------------------------------------------------------------
    ENGINE_API static uint64_t GetTimeMicroseconds();
    ENGINE_API static double GetTimeSeconds();

    // ------------------------------------------------------------
    // 输入管理
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // 文件系统
    // ------------------------------------------------------------
    ENGINE_API static bool FileExists(const char* path);
    ENGINE_API static size_t FileSize(const char* path);
    ENGINE_API static size_t ReadFile(const char* path, void* dst, size_t maxBytes);
    ENGINE_API static const char* GetExecutablePath();
    ENGINE_API static const char* GetPersistentPath();
    ENGINE_API static const char* GetTemporaryPath();

    // ------------------------------------------------------------
    // 线程和同步
    // ------------------------------------------------------------
    ENGINE_API static PlatformThreadHandle CreateThread(ThreadFunc entry, void* userData);
    ENGINE_API static void JoinThread(PlatformThreadHandle thread);
    ENGINE_API static PlatformMutexHandle CreateMutex();
    ENGINE_API static void DestroyMutex(PlatformMutexHandle mtx);
    ENGINE_API static void LockMutex(PlatformMutexHandle mtx);
    ENGINE_API static void UnlockMutex(PlatformMutexHandle mtx);
    ENGINE_API static void SleepMilliseconds(uint32_t ms);

    // ------------------------------------------------------------
    // Vulkan 支持
    // ------------------------------------------------------------
    ENGINE_API static std::vector<const char*> GetVulkanInstanceExtensions();
    ENGINE_API static bool CreateVulkanSurface(void* instance, WindowHandle windowHandle, void** outSurface);

    // ------------------------------------------------------------
    // IPlatformLogger 接口实现
    // ------------------------------------------------------------
    ENGINE_API static void LogToConsole(LogLevel level, const char* tag, const char* message);
    ENGINE_API static const char* GetLogDirectoryPath();

    // ------------------------------------------------------------
    // SDL 特定功能
    // ------------------------------------------------------------
    ENGINE_API static void SetEventCallback(EventCallback callback);
};

} // namespace Prisma