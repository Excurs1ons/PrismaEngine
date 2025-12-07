#pragma once
#include "Platform.h"
#include "ManagerBase.h"
#include <SDL3/SDL.h>
#include <vector>
#include <functional>

namespace Engine {

class PlatformSDL : public Platform, public ManagerBase<PlatformSDL> {
public:
    friend class ManagerBase<PlatformSDL>;

    PlatformSDL();
    ~PlatformSDL() override;

    // 平台生命周期管理
    bool Initialize() override;
    void Shutdown() override;

    // 窗口管理
    WindowHandle CreateWindow(const WindowProps& desc) override;
    void DestroyWindow(WindowHandle window) override;
    void GetWindowSize(WindowHandle window, int& outW, int& outH) override;
    void SetWindowTitle(WindowHandle window, const char* title) override;
    void PumpEvents() override;
    bool ShouldClose(WindowHandle window) const override;

    using EventCallback = std::function<bool(const SDL_Event*)>;
    void SetEventCallback(EventCallback callback);

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

    // Vulkan 支持
    std::vector<const char*> GetVulkanInstanceExtensions() const override;
    bool CreateVulkanSurface(void* instance, WindowHandle windowHandle, void** outSurface) override;

private:
    bool m_shouldClose = false;
    bool m_initialized = false;
    EventCallback m_eventCallback;
};

} // namespace Engine
