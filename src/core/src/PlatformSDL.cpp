#include "PlatformSDL.h"
#include "Logger.h"
#include <SDL3/SDL_vulkan.h>
#include <thread>
#include <chrono>
#include <vulkan/vulkan.h>

namespace Engine {

PlatformSDL::PlatformSDL() {
}

PlatformSDL::~PlatformSDL() {
    Shutdown();
}

bool PlatformSDL::Initialize() {
    if (m_initialized) return true;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_FATAL("PlatformSDL", "Failed to initialize SDL: {0}", SDL_GetError());
        return false;
    } else {
        LOG_INFO("PlatformSDL", "SDL initialized successfully");
        m_initialized = true;
        return true;
    }
}

void PlatformSDL::Shutdown() {
    if (m_initialized) {
        SDL_Quit();
        m_initialized = false;
        LOG_INFO("PlatformSDL", "SDL shutdown");
    }
}

WindowHandle PlatformSDL::CreateWindow(const WindowProps& desc) {
    Uint32 flags = SDL_WINDOW_RESIZABLE;
    if (desc.FullScreenMode == FullScreenMode::FullScreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    
    // 默认支持 Vulkan，因为我们主要用它做 Vulkan 后端
    flags |= SDL_WINDOW_VULKAN;
    flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

    if (desc.ShowState == WindowShowState::Hide) {
        flags |= SDL_WINDOW_HIDDEN;
    } else if (desc.ShowState == WindowShowState::Maximize) {
        flags |= SDL_WINDOW_MAXIMIZED;
    } else if (desc.ShowState == WindowShowState::Minimize) {
        flags |= SDL_WINDOW_MINIMIZED;
    }

    SDL_Window* window = SDL_CreateWindow(desc.Title.c_str(), desc.Width, desc.Height, flags);
    if (!window) {
        LOG_ERROR("PlatformSDL", "Failed to create window: {0}", SDL_GetError());
        return nullptr;
    }
    return window;
}

void PlatformSDL::DestroyWindow(WindowHandle window) {
    if (window) {
        SDL_DestroyWindow(static_cast<SDL_Window*>(window));
    }
}

void PlatformSDL::GetWindowSize(WindowHandle window, int& outW, int& outH) {
    if (window) {
        SDL_GetWindowSize(static_cast<SDL_Window*>(window), &outW, &outH);
    }
}

void PlatformSDL::SetWindowTitle(WindowHandle window, const char* title) {
    if (window) {
        SDL_SetWindowTitle(static_cast<SDL_Window*>(window), title);
    }
}

void PlatformSDL::SetEventCallback(EventCallback callback) {
    m_eventCallback = callback;
}

void PlatformSDL::PumpEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (m_eventCallback) {
            if (m_eventCallback(&event)) {
                continue; // 如果回调处理了事件，就不再继续处理
            }
        }

        if (event.type == SDL_EVENT_QUIT) {
            m_shouldClose = true;
        }
        // 这里可以添加输入事件处理，或者让 InputManager 处理
    }
}

bool PlatformSDL::ShouldClose(WindowHandle window) const {
    return m_shouldClose;
}

uint64_t PlatformSDL::GetTimeMicroseconds() const {
    return SDL_GetTicksNS() / 1000;
}

double PlatformSDL::GetTimeSeconds() const {
    return SDL_GetTicks() / 1000.0;
}

bool PlatformSDL::IsKeyDown(KeyCode key) const {
    // 简单的映射，实际可能需要更复杂的 KeyCode 转换
    const bool* state = SDL_GetKeyboardState(NULL);
    // TODO: 实现 KeyCode 到 SDL_Scancode 的映射
    return false; 
}

bool PlatformSDL::IsMouseButtonDown(MouseButton btn) const {
    Uint32 state = SDL_GetMouseState(NULL, NULL);
    if (btn == 0) return (state & SDL_BUTTON_LMASK) != 0;
    if (btn == 1) return (state & SDL_BUTTON_RMASK) != 0;
    if (btn == 2) return (state & SDL_BUTTON_MMASK) != 0;
    return false;
}

void PlatformSDL::GetMousePosition(float& x, float& y) const {
    SDL_GetMouseState(&x, &y);
}

void PlatformSDL::SetMousePosition(float x, float y) {
    // 需要 active window 才能设置鼠标位置，这里简化处理
    // SDL_WarpMouseInWindow(window, x, y);
}

void PlatformSDL::SetMouseLock(bool locked) {
    SDL_SetWindowRelativeMouseMode(NULL, locked ? true : false);
}

bool PlatformSDL::FileExists(const char* path) const {
    // SDL3 没有直接的 FileExists，使用标准库或 SDL_IOStream
    SDL_IOStream* rw = SDL_IOFromFile(path, "rb");
    if (rw) {
        SDL_CloseIO(rw);
        return true;
    }
    return false;
}

size_t PlatformSDL::FileSize(const char* path) const {
    SDL_IOStream* rw = SDL_IOFromFile(path, "rb");
    if (!rw) return 0;
    Sint64 size = SDL_GetIOSize(rw);
    SDL_CloseIO(rw);
    return (size > 0) ? (size_t)size : 0;
}

size_t PlatformSDL::ReadFile(const char* path, void* dst, size_t maxBytes) const {
    SDL_IOStream* rw = SDL_IOFromFile(path, "rb");
    if (!rw) return 0;
    size_t read = SDL_ReadIO(rw, dst, maxBytes);
    SDL_CloseIO(rw);
    return read;
}

const char* PlatformSDL::GetExecutablePath() const {
    return SDL_GetBasePath();
}

const char* PlatformSDL::GetPersistentPath() const {
    return SDL_GetPrefPath("YAGE", "Engine");
}

const char* PlatformSDL::GetTemporaryPath() const {
    return nullptr; // SDL3 没有直接的临时路径 API
}

PlatformThreadHandle PlatformSDL::CreateThread(ThreadFunc entry, void* userData) {
    return SDL_CreateThread((SDL_ThreadFunction)entry, "YAGE_Thread", userData);
}

void PlatformSDL::JoinThread(PlatformThreadHandle thread) {
    SDL_WaitThread(static_cast<SDL_Thread*>(thread), NULL);
}

PlatformMutexHandle PlatformSDL::CreateMutex() {
    return SDL_CreateMutex();
}

void PlatformSDL::DestroyMutex(PlatformMutexHandle mtx) {
    SDL_DestroyMutex(static_cast<SDL_Mutex*>(mtx));
}

void PlatformSDL::LockMutex(PlatformMutexHandle mtx) {
    SDL_LockMutex(static_cast<SDL_Mutex*>(mtx));
}

void PlatformSDL::UnlockMutex(PlatformMutexHandle mtx) {
    SDL_UnlockMutex(static_cast<SDL_Mutex*>(mtx));
}

void PlatformSDL::SleepMilliseconds(uint32_t ms) {
    SDL_Delay(ms);
}

std::vector<const char*> PlatformSDL::GetVulkanInstanceExtensions() const {
    uint32_t count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!extensions) return {};
    std::vector<const char*> result;
    for (uint32_t i = 0; i < count; i++) {
        result.push_back(extensions[i]);
    }
    return result;
}

bool PlatformSDL::CreateVulkanSurface(void* instance, WindowHandle windowHandle, void** outSurface) {
    if (!instance || !windowHandle || !outSurface) return false;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (SDL_Vulkan_CreateSurface(static_cast<SDL_Window*>(windowHandle), static_cast<VkInstance>(instance), nullptr, &surface)) {
        *outSurface = (void*)surface;
        return true;
    }
    LOG_ERROR("PlatformSDL", "Failed to create Vulkan surface: {0}", SDL_GetError());
    return false;
}

} // namespace Engine
