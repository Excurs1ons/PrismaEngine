//// x86
//#ifdef _M_IX86
//    // 32λ x86
//#if _M_IX86 == 600
//    // Pentium Pro/II/III
//#endif
//#endif
//
//// x64 �ܹ����
//#ifdef _M_X64
//    // 64λ x86 (AMD64/Intel64)
//#endif
//
//#ifdef _M_ARM
//#if defined(_M_ARM_NT) || defined(_M_ARM64)
//#endif
//#endif
//
//
//#ifdef _M_ARM64
//#endif
//
//#ifdef _WIN32
//#ifdef _WIN64
//#else
//#endif
//#endif
//
//// Universal Windows Platform (UWP)
//#ifdef _WIN32
//#ifdef WINAPI_FAMILY
//#include <winapifamily.h>
//#if WINAPI_FAMILY == WINAPI_FAMILY_PC_APP || WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
//#endif
//#endif
//#endif

#pragma once

#include "ManagerBase.h"

#include <KeyCode.h>
#include <string>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#ifdef CreateWindow
#undef CreateWindow
#endif
#include <Logger.h>
#ifdef CreateMutex
#undef CreateMutex
#endif
#endif

class Time {
public:
    static float DeltaTime;  // 这一帧的 dt
    static float TotalTime;  // 游戏总时长
    static float TimeScale;  // 时间缩放 (1.0 = 正常, 0.5 = 慢放)

    // 获取自程序启动以来的秒数
    static float GetTime() {
        using namespace std::chrono;
        static auto start = high_resolution_clock::now();
        auto now          = high_resolution_clock::now();
        return duration<float>(now - start).count();
    }
};


enum class FullScreenMode { Window, ExclusiveFullScreen, FullScreen };
// 添加窗口创建标志枚举
enum class WindowShowState { Default, Show, Hide, Maximize, Minimize };

// --- Window (抽象基类) ---
struct WindowProps {
    std::string Title;
    uint32_t Width;
    uint32_t Height;
    bool Resizable                = false;
    FullScreenMode FullScreenMode = FullScreenMode::Window;
    WindowShowState ShowState     = WindowShowState::Default;
    WindowProps(std::string t = "Engine", uint32_t w = 1280, uint32_t h = 720) : Title(t), Width(w), Height(h) {}
};

using WindowHandle = void*;
namespace Engine {
using namespace Input;
// ------------------------------------------
// ------------------------------------------
using PlatformThreadHandle = void*;
using PlatformMutexHandle  = void*;

using ThreadFunc = void* (*)(void*);

class Platform {
public:
    virtual ~Platform() = default;

    // ------------------------------------------------------------
    // 平台生命周期管理
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // 窗口管理
    // ------------------------------------------------------------
    virtual WindowHandle CreateWindow(const WindowProps& desc) = 0;
    virtual void DestroyWindow(WindowHandle window) = 0;
    virtual void GetWindowSize(WindowHandle window, int& outW, int& outH) = 0;
    virtual void SetWindowTitle(WindowHandle window, const char* title) = 0;
    virtual void PumpEvents() = 0;
    virtual bool ShouldClose(WindowHandle window) const = 0;

    // ------------------------------------------------------------
    // 时间管理
    // ------------------------------------------------------------
    virtual uint64_t GetTimeMicroseconds() const = 0;
    virtual double GetTimeSeconds() const = 0;

    // ------------------------------------------------------------
    // 输入管理
    // ------------------------------------------------------------
    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsMouseButtonDown(MouseButton btn) const = 0;
    virtual void GetMousePosition(float& x, float& y) const = 0;
    virtual void SetMousePosition(float x, float y) = 0;
    virtual void SetMouseLock(bool locked) = 0;

    // ------------------------------------------------------------
    // 文件系统
    // ------------------------------------------------------------
    virtual bool FileExists(const char* path) const = 0;
    virtual size_t FileSize(const char* path) const = 0;
    virtual size_t ReadFile(const char* path, void* dst, size_t maxBytes) const = 0;
    virtual const char* GetExecutablePath() const = 0;
    virtual const char* GetPersistentPath() const = 0;
    virtual const char* GetTemporaryPath() const = 0;

    // ------------------------------------------------------------
    // 线程和同步
    // ------------------------------------------------------------
    virtual PlatformThreadHandle CreateThread(ThreadFunc entry, void* userData) = 0;
    virtual void JoinThread(PlatformThreadHandle thread) = 0;
    virtual PlatformMutexHandle CreateMutex() = 0;
    virtual void DestroyMutex(PlatformMutexHandle mtx) = 0;
    virtual void LockMutex(PlatformMutexHandle mtx) = 0;
    virtual void UnlockMutex(PlatformMutexHandle mtx) = 0;
    virtual void SleepMilliseconds(uint32_t ms) = 0;

    // ------------------------------------------------------------
    // Vulkan 支持
    // ------------------------------------------------------------
    virtual std::vector<const char*> GetVulkanInstanceExtensions() const { return {}; }
    virtual bool CreateVulkanSurface(void* instance, WindowHandle windowHandle, void** outSurface) { return false; }
};
}  // namespace Engine