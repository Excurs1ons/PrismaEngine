#pragma once

#include "Export.h"
#include "IPlatformLogger.h"
#include "input/InputManager.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

// 条件包含平台特定头文件
#ifdef _WIN32
#include <Windows.h>
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef CreateMutex
#undef CreateMutex
#endif
#endif


// SDL 相关
#if defined(__has_include)
    #if __has_include(<SDL3/SDL.h>)
        #include <SDL3/SDL.h>
        #define PRISMA_HAS_SDL 1
    #endif
#endif

// ------------------------------------------------------------
// 时间类 - 独立定义，避免依赖 chrono
// ------------------------------------------------------------
class Time {
public:
    static float DeltaTime;
    static float TotalTime;
    static float TimeScale;

    static float GetTime();
};

// ------------------------------------------------------------
// 窗口相关枚举和结构
// ------------------------------------------------------------
enum class FullScreenMode { Window, ExclusiveFullScreen, FullScreen };
enum class WindowShowState { Default, Show, Hide, Maximize, Minimize };

struct WindowProps {
    std::string Title;
    uint32_t Width;
    uint32_t Height;
    bool Resizable                = false;
    FullScreenMode fullScreenMode = FullScreenMode::Window;
    WindowShowState ShowState     = WindowShowState::Default;

    WindowProps(std::string t = "Engine", uint32_t w = 1280, uint32_t h = 720)
        : Title(t), Width(w), Height(h) {}
};

using WindowHandle = void*;
// ------------------------------------------------------------
// 类型定义
// ------------------------------------------------------------
using PlatformThreadHandle = void*;
using PlatformMutexHandle  = void*;
using ThreadFunc = void* (*)(void*);
// ------------------------------------------------------------
// Platform - 静态平台抽象层
// 所有函数都是静态的，使用宏控制平台实现
// ------------------------------------------------------------
namespace PrismaEngine {

class Platform {
public:
    static bool s_initialized;
    static bool s_shouldClose;
    static WindowHandle s_currentWindow;

    using EventCallback = std::function<bool(const void*)>;
    static EventCallback s_eventCallback;

    // ------------------------------------------------------------
    // 平台生命周期管理
    // ------------------------------------------------------------
    ENGINE_API static bool Initialize();
    ENGINE_API static void Shutdown();
    ENGINE_API static bool IsInitialized();

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

#if defined(_WIN32)
    ENGINE_API static bool SetWindowIcon(const std::string& path);
#endif

    // ------------------------------------------------------------
    // 时间管理
    // ------------------------------------------------------------
    ENGINE_API static uint64_t GetTimeMicroseconds();
    ENGINE_API static double GetTimeSeconds();

    // ------------------------------------------------------------
    // 输入管理
    // ------------------------------------------------------------
#if defined(_WIN32) || defined(PRISMA_HAS_SDL)
    ENGINE_API static bool IsKeyDown(PrismaEngine::Input::KeyCode key);
    ENGINE_API static bool IsMouseButtonDown(PrismaEngine::Input::MouseButton btn);
    ENGINE_API static void GetMousePosition(float& x, float& y);
    ENGINE_API static void SetMousePosition(float x, float y);
    ENGINE_API static void SetMouseLock(bool locked);
#endif

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

} // namespace PrismaEngine