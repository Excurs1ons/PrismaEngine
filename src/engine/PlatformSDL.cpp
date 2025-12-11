#include "PlatformSDL.h"
#include "Logger.h"
#include <SDL3/SDL_vulkan.h>
#include <chrono>
#include <thread>
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
    const bool* state = SDL_GetKeyboardState(NULL);

    // KeyCode 到 SDL_Scancode 的映射
    switch (key) {
        // 字母键
        case KeyCode::A: return state[SDL_SCANCODE_A];
        case KeyCode::B: return state[SDL_SCANCODE_B];
        case KeyCode::C: return state[SDL_SCANCODE_C];
        case KeyCode::D: return state[SDL_SCANCODE_D];
        case KeyCode::E: return state[SDL_SCANCODE_E];
        case KeyCode::F: return state[SDL_SCANCODE_F];
        case KeyCode::G: return state[SDL_SCANCODE_G];
        case KeyCode::H: return state[SDL_SCANCODE_H];
        case KeyCode::I: return state[SDL_SCANCODE_I];
        case KeyCode::J: return state[SDL_SCANCODE_J];
        case KeyCode::K: return state[SDL_SCANCODE_K];
        case KeyCode::L: return state[SDL_SCANCODE_L];
        case KeyCode::M: return state[SDL_SCANCODE_M];
        case KeyCode::N: return state[SDL_SCANCODE_N];
        case KeyCode::O: return state[SDL_SCANCODE_O];
        case KeyCode::P: return state[SDL_SCANCODE_P];
        case KeyCode::Q: return state[SDL_SCANCODE_Q];
        case KeyCode::R: return state[SDL_SCANCODE_R];
        case KeyCode::S: return state[SDL_SCANCODE_S];
        case KeyCode::T: return state[SDL_SCANCODE_T];
        case KeyCode::U: return state[SDL_SCANCODE_U];
        case KeyCode::V: return state[SDL_SCANCODE_V];
        case KeyCode::W: return state[SDL_SCANCODE_W];
        case KeyCode::X: return state[SDL_SCANCODE_X];
        case KeyCode::Y: return state[SDL_SCANCODE_Y];
        case KeyCode::Z: return state[SDL_SCANCODE_Z];

        // 数字键
        case KeyCode::Num0: return state[SDL_SCANCODE_0];
        case KeyCode::Num1: return state[SDL_SCANCODE_1];
        case KeyCode::Num2: return state[SDL_SCANCODE_2];
        case KeyCode::Num3: return state[SDL_SCANCODE_3];
        case KeyCode::Num4: return state[SDL_SCANCODE_4];
        case KeyCode::Num5: return state[SDL_SCANCODE_5];
        case KeyCode::Num6: return state[SDL_SCANCODE_6];
        case KeyCode::Num7: return state[SDL_SCANCODE_7];
        case KeyCode::Num8: return state[SDL_SCANCODE_8];
        case KeyCode::Num9: return state[SDL_SCANCODE_9];

        // 功能键
        case KeyCode::F1: return state[SDL_SCANCODE_F1];
        case KeyCode::F2: return state[SDL_SCANCODE_F2];
        case KeyCode::F3: return state[SDL_SCANCODE_F3];
        case KeyCode::F4: return state[SDL_SCANCODE_F4];
        case KeyCode::F5: return state[SDL_SCANCODE_F5];
        case KeyCode::F6: return state[SDL_SCANCODE_F6];
        case KeyCode::F7: return state[SDL_SCANCODE_F7];
        case KeyCode::F8: return state[SDL_SCANCODE_F8];
        case KeyCode::F9: return state[SDL_SCANCODE_F9];
        case KeyCode::F10: return state[SDL_SCANCODE_F10];
        case KeyCode::F11: return state[SDL_SCANCODE_F11];
        case KeyCode::F12: return state[SDL_SCANCODE_F12];

        // 方向键
        case KeyCode::ArrowUp: return state[SDL_SCANCODE_UP];
        case KeyCode::ArrowDown: return state[SDL_SCANCODE_DOWN];
        case KeyCode::ArrowLeft: return state[SDL_SCANCODE_LEFT];
        case KeyCode::ArrowRight: return state[SDL_SCANCODE_RIGHT];

        // 特殊键
        case KeyCode::Space: return state[SDL_SCANCODE_SPACE];
        case KeyCode::Enter: return state[SDL_SCANCODE_RETURN];
        case KeyCode::Escape: return state[SDL_SCANCODE_ESCAPE];
        case KeyCode::Backspace: return state[SDL_SCANCODE_BACKSPACE];
        case KeyCode::Tab: return state[SDL_SCANCODE_TAB];
        case KeyCode::CapsLock: return state[SDL_SCANCODE_CAPSLOCK];

        // 修饰键
        case KeyCode::LeftShift: return state[SDL_SCANCODE_LSHIFT];
        case KeyCode::RightShift: return state[SDL_SCANCODE_RSHIFT];
        case KeyCode::LeftControl: return state[SDL_SCANCODE_LCTRL];
        case KeyCode::RightControl: return state[SDL_SCANCODE_RCTRL];
        case KeyCode::LeftAlt: return state[SDL_SCANCODE_LALT];
        case KeyCode::RightAlt: return state[SDL_SCANCODE_RALT];

        // 符号键
        case KeyCode::Grave: return state[SDL_SCANCODE_GRAVE];
        case KeyCode::Minus: return state[SDL_SCANCODE_MINUS];
        case KeyCode::Equal: return state[SDL_SCANCODE_EQUALS];
        case KeyCode::LeftBracket: return state[SDL_SCANCODE_LEFTBRACKET];
        case KeyCode::RightBracket: return state[SDL_SCANCODE_RIGHTBRACKET];
        case KeyCode::Backslash: return state[SDL_SCANCODE_BACKSLASH];
        case KeyCode::Semicolon: return state[SDL_SCANCODE_SEMICOLON];
        case KeyCode::Apostrophe: return state[SDL_SCANCODE_APOSTROPHE];
        case KeyCode::Comma: return state[SDL_SCANCODE_COMMA];
        case KeyCode::Period: return state[SDL_SCANCODE_PERIOD];
        case KeyCode::Slash: return state[SDL_SCANCODE_SLASH];

        default: return false;
    }
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
