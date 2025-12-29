#pragma once

#include "IPlatformLogger.h"
#include "KeyCode.h"
#include <string>
#include <vector>
#include <cstdint>

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


// SDL 相关（仅在非 Windows/Android 平台）
#if !defined(_WIN32) && !defined(__ANDROID__)
    #if defined(__has_include)
        #if __has_include(<SDL3/SDL.h>)
            #include <SDL3/SDL.h>
            #define PRISMA_HAS_SDL 1
        #endif
    #endif
    #include <functional>
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
    FullScreenMode FullScreenMode = FullScreenMode::Window;
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
namespace Engine {

class Platform {
public:
    static bool s_initialized;
    static bool s_shouldClose;
    static WindowHandle s_currentWindow;


#if !defined(_WIN32) && !defined(__ANDROID__)
    using EventCallback = std::function<bool(const void*)>;
#endif

    // ------------------------------------------------------------
    // 平台生命周期管理
    // ------------------------------------------------------------
    static bool Initialize();
    static void Shutdown();
    static bool IsInitialized();

    // ------------------------------------------------------------
    // 窗口管理
    // ------------------------------------------------------------
    static WindowHandle CreateWindow(const WindowProps& desc);
    static void DestroyWindow(WindowHandle window);
    static void GetWindowSize(WindowHandle window, int& outW, int& outH);
    static void SetWindowTitle(WindowHandle window, const char* title);
    static void PumpEvents();
    static bool ShouldClose(WindowHandle window);
    static WindowHandle GetCurrentWindow();

#if defined(_WIN32)
    static bool SetWindowIcon(const std::string& path);
#endif

    // ------------------------------------------------------------
    // 时间管理
    // ------------------------------------------------------------
    static uint64_t GetTimeMicroseconds();
    static double GetTimeSeconds();

    // ------------------------------------------------------------
    // 输入管理（需要 KeyCode 支持）
    // ------------------------------------------------------------
#if defined(PRISMA_HAS_KEYCODE) || defined(_WIN32) || defined(__ANDROID__)
    static bool IsKeyDown(Engine::Input::KeyCode key);
    static bool IsMouseButtonDown(Engine::Input::MouseButton btn);
    static void GetMousePosition(float& x, float& y);
    static void SetMousePosition(float x, float y);
    static void SetMouseLock(bool locked);
#endif

    // ------------------------------------------------------------
    // 文件系统
    // ------------------------------------------------------------
    static bool FileExists(const char* path);
    static size_t FileSize(const char* path);
    static size_t ReadFile(const char* path, void* dst, size_t maxBytes);
    static const char* GetExecutablePath();
    static const char* GetPersistentPath();
    static const char* GetTemporaryPath();

    // ------------------------------------------------------------
    // 线程和同步
    // ------------------------------------------------------------
    static PlatformThreadHandle CreateThread(ThreadFunc entry, void* userData);
    static void JoinThread(PlatformThreadHandle thread);
    static PlatformMutexHandle CreateMutex();
    static void DestroyMutex(PlatformMutexHandle mtx);
    static void LockMutex(PlatformMutexHandle mtx);
    static void UnlockMutex(PlatformMutexHandle mtx);
    static void SleepMilliseconds(uint32_t ms);

    // ------------------------------------------------------------
    // Vulkan 支持
    // ------------------------------------------------------------
    static std::vector<const char*> GetVulkanInstanceExtensions();
    static bool CreateVulkanSurface(void* instance, WindowHandle windowHandle, void** outSurface);

    // ------------------------------------------------------------
    // IPlatformLogger 接口实现
    // ------------------------------------------------------------
    static void LogToConsole(PlatformLogLevel level, const char* tag, const char* message);
    static const char* GetLogDirectoryPath();

    // ------------------------------------------------------------
    // SDL 特定功能
    // ------------------------------------------------------------
#if !defined(_WIN32) && !defined(__ANDROID__)
    static void SetEventCallback(EventCallback callback);
#endif
};

} // namespace Engine
