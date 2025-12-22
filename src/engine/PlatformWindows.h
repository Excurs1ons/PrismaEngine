#pragma once

#ifdef _WIN32
#define NOMINMAX
#include "Platform.h"
#include <Windows.h>
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef CreateMutex
#undef CreateMutex
#endif

namespace Engine {

class PlatformWindows : public Platform,public ManagerBase<PlatformWindows>{
public:
    friend class ManagerBase<PlatformWindows>;
    // 平台生命周期管理
    bool Initialize() override;
    void Shutdown() override;

    WindowHandle GetWindowHandle() const;
    // 窗口管理
    WindowHandle CreateWindow(const WindowProps& desc) override;
    void DestroyWindow(WindowHandle window) override;
    void GetWindowSize(WindowHandle window, int& outW, int& outH) override;
    void SetWindowTitle(WindowHandle window, const char* title) override;
    void PumpEvents() override;
    bool ShouldClose(WindowHandle window) const override;
    
    // 窗口图标设置（Windows特有功能）
    bool SetWindowIcon(const std::string& path);

    // 时间管理
    uint64_t GetTimeMicroseconds() const override;
    double GetTimeSeconds() const override;

    // 输入管理
    bool IsKeyDown(KeyCode key) const override;
    bool IsMouseButtonDown(MouseButton btn) const override;
    void GetMousePosition(float& x, float& y) const override;
    void SetMousePosition(float x, float y) override;
    void SetMouseLock(bool locked) override;

    // 文件系统
    bool FileExists(const char* path) const override;
    size_t FileSize(const char* path) const override;
    size_t ReadFile(const char* path, void* dst, size_t maxBytes) const override;
    const char* GetExecutablePath() const override;
    const char* GetPersistentPath() const override;
    const char* GetTemporaryPath() const override;

    // 线程和同步
    PlatformThreadHandle CreateThread(ThreadFunc entry, void* userData) override;
    void JoinThread(PlatformThreadHandle thread) override;
    PlatformMutexHandle CreateMutex() override;
    void DestroyMutex(PlatformMutexHandle mtx) override;
    void LockMutex(PlatformMutexHandle mtx) override;
    void UnlockMutex(PlatformMutexHandle mtx) override;
    void SleepMilliseconds(uint32_t ms) override;

    PlatformWindows();
private:
    HWND hwnd = nullptr;  // 当前窗口句柄
};

}  // namespace Engine

#endif // _WIN32
