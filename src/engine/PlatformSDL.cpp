#include "Platform.h"

// SDL 用于非 Windows 平台（Linux, macOS 等）
// Windows 使用 PlatformWindows.cpp，Android 使用 PlatformAndroid.cpp
#if !defined(_WIN32) && !defined(__ANDROID__)

// 条件包含 SDL 头文件
#if defined(__has_include)
    #if __has_include(<SDL3/SDL.h>)
        #include <SDL3/SDL.h>
        #include <SDL3/SDL_vulkan.h>
        #define PRISMA_SDL_AVAILABLE 1
    #endif
#else
    // 如果不支持 __has_include，假设 SDL 可用
    #include <SDL3/SDL.h>
    #include <SDL3/SDL_vulkan.h>
    #define PRISMA_SDL_AVAILABLE 1
#endif

#if defined(PRISMA_SDL_AVAILABLE)

#include <chrono>
#include <iostream>
#include <thread>
#include <vulkan/vulkan.h>

namespace Engine {

// ------------------------------------------------------------
// SDL 平台静态变量
// ------------------------------------------------------------
static bool s_sdlInitialized = false;

// ------------------------------------------------------------
// 平台生命周期管理
// ------------------------------------------------------------
bool Platform::Initialize() {
    if (s_initialized) {
        return true;
    }

    if (!s_sdlInitialized) {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            std::cerr << "[PlatformSDL] Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }
        s_sdlInitialized = true;
    }

    s_initialized = true;
    s_shouldClose = false;
    return true;
}

void Platform::Shutdown() {
    if (!s_initialized) {
        return;
    }

    if (s_sdlInitialized) {
        SDL_Quit();
        s_sdlInitialized = false;
    }

    s_initialized = false;
    s_currentWindow = nullptr;
}

// ------------------------------------------------------------
// SDL 特定功能
// ------------------------------------------------------------
void Platform::SetEventCallback(EventCallback callback) {
    s_eventCallback = callback;
}

// ------------------------------------------------------------
// 窗口管理
// ------------------------------------------------------------
WindowHandle Platform::CreateWindow(const WindowProps& desc) {
    Uint32 flags = SDL_WINDOW_RESIZABLE;
    if (desc.FullScreenMode == FullScreenMode::FullScreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    // 默认支持 Vulkan
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
        std::cerr << "[PlatformSDL] Failed to create window: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    s_currentWindow = window;
    return window;
}

void Platform::DestroyWindow(WindowHandle window) {
    if (window) {
        SDL_DestroyWindow(static_cast<SDL_Window*>(window));
    }
    if (s_currentWindow == window) {
        s_currentWindow = nullptr;
    }
}

void Platform::GetWindowSize(WindowHandle window, int& outW, int& outH) {
    if (window) {
        SDL_GetWindowSize(static_cast<SDL_Window*>(window), &outW, &outH);
    }
}

void Platform::SetWindowTitle(WindowHandle window, const char* title) {
    if (window) {
        SDL_SetWindowTitle(static_cast<SDL_Window*>(window), title);
    }
}

void Platform::PumpEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (s_eventCallback) {
            if (s_eventCallback(&event)) {
                continue;
            }
        }

        if (event.type == SDL_EVENT_QUIT) {
            s_shouldClose = true;
        }
    }
}

bool Platform::ShouldClose(WindowHandle window) {
    (void)window;
    return s_shouldClose;
}

// ------------------------------------------------------------
// 时间管理
// ------------------------------------------------------------
uint64_t Platform::GetTimeMicroseconds() {
    return SDL_GetTicksNS() / 1000;
}

double Platform::GetTimeSeconds() {
    return SDL_GetTicks() / 1000.0;
}

// ------------------------------------------------------------
// 输入管理
// ------------------------------------------------------------
#if defined(PRISMA_HAS_KEYCODE)
bool Platform::IsKeyDown(KeyCode key) {
    const bool* state = SDL_GetKeyboardState(NULL);

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
        case KeyCode::F1:  return state[SDL_SCANCODE_F1];
        case KeyCode::F2:  return state[SDL_SCANCODE_F2];
        case KeyCode::F3:  return state[SDL_SCANCODE_F3];
        case KeyCode::F4:  return state[SDL_SCANCODE_F4];
        case KeyCode::F5:  return state[SDL_SCANCODE_F5];
        case KeyCode::F6:  return state[SDL_SCANCODE_F6];
        case KeyCode::F7:  return state[SDL_SCANCODE_F7];
        case KeyCode::F8:  return state[SDL_SCANCODE_F8];
        case KeyCode::F9:  return state[SDL_SCANCODE_F9];
        case KeyCode::F10: return state[SDL_SCANCODE_F10];
        case KeyCode::F11: return state[SDL_SCANCODE_F11];
        case KeyCode::F12: return state[SDL_SCANCODE_F12];

        // 方向键
        case KeyCode::ArrowUp:    return state[SDL_SCANCODE_UP];
        case KeyCode::ArrowDown:  return state[SDL_SCANCODE_DOWN];
        case KeyCode::ArrowLeft:  return state[SDL_SCANCODE_LEFT];
        case KeyCode::ArrowRight: return state[SDL_SCANCODE_RIGHT];

        // 特殊键
        case KeyCode::Space:     return state[SDL_SCANCODE_SPACE];
        case KeyCode::Enter:     return state[SDL_SCANCODE_RETURN];
        case KeyCode::Escape:    return state[SDL_SCANCODE_ESCAPE];
        case KeyCode::Backspace: return state[SDL_SCANCODE_BACKSPACE];
        case KeyCode::Tab:       return state[SDL_SCANCODE_TAB];
        case KeyCode::CapsLock:  return state[SDL_SCANCODE_CAPSLOCK];

        // 修饰键
        case KeyCode::LeftShift:  return state[SDL_SCANCODE_LSHIFT];
        case KeyCode::RightShift: return state[SDL_SCANCODE_RSHIFT];
        case KeyCode::LeftControl: return state[SDL_SCANCODE_LCTRL];
        case KeyCode::RightControl: return state[SDL_SCANCODE_RCTRL];
        case KeyCode::LeftAlt:    return state[SDL_SCANCODE_LALT];
        case KeyCode::RightAlt:   return state[SDL_SCANCODE_RALT];

        // 符号键
        case KeyCode::Grave:      return state[SDL_SCANCODE_GRAVE];
        case KeyCode::Minus:      return state[SDL_SCANCODE_MINUS];
        case KeyCode::Equal:      return state[SDL_SCANCODE_EQUALS];
        case KeyCode::LeftBracket:  return state[SDL_SCANCODE_LEFTBRACKET];
        case KeyCode::RightBracket: return state[SDL_SCANCODE_RIGHTBRACKET];
        case KeyCode::Backslash:  return state[SDL_SCANCODE_BACKSLASH];
        case KeyCode::Semicolon:  return state[SDL_SCANCODE_SEMICOLON];
        case KeyCode::Apostrophe: return state[SDL_SCANCODE_APOSTROPHE];
        case KeyCode::Comma:      return state[SDL_SCANCODE_COMMA];
        case KeyCode::Period:     return state[SDL_SCANCODE_PERIOD];
        case KeyCode::Slash:      return state[SDL_SCANCODE_SLASH];

        default: return false;
    }
}

bool Platform::IsMouseButtonDown(MouseButton btn) {
    Uint32 state = SDL_GetMouseState(NULL, NULL);
    if (btn == 0) return (state & SDL_BUTTON_LMASK) != 0;
    if (btn == 1) return (state & SDL_BUTTON_RMASK) != 0;
    if (btn == 2) return (state & SDL_BUTTON_MMASK) != 0;
    return false;
}

void Platform::GetMousePosition(float& x, float& y) {
    SDL_GetMouseState(&x, &y);
}

void Platform::SetMousePosition(float x, float y) {
    (void)x;
    (void)y;
}

void Platform::SetMouseLock(bool locked) {
    SDL_SetWindowRelativeMouseMode(NULL, locked ? true : false);
}

#endif // PRISMA_HAS_KEYCODE

// ------------------------------------------------------------
// 文件系统
// ------------------------------------------------------------
bool Platform::FileExists(const char* path) {
    SDL_IOStream* rw = SDL_IOFromFile(path, "rb");
    if (rw) {
        SDL_CloseIO(rw);
        return true;
    }
    return false;
}

size_t Platform::FileSize(const char* path) {
    SDL_IOStream* rw = SDL_IOFromFile(path, "rb");
    if (!rw) return 0;
    Sint64 size = SDL_GetIOSize(rw);
    SDL_CloseIO(rw);
    return (size > 0) ? (size_t)size : 0;
}

size_t Platform::ReadFile(const char* path, void* dst, size_t maxBytes) {
    SDL_IOStream* rw = SDL_IOFromFile(path, "rb");
    if (!rw) return 0;
    size_t read = SDL_ReadIO(rw, dst, maxBytes);
    SDL_CloseIO(rw);
    return read;
}

const char* Platform::GetExecutablePath() {
    return SDL_GetBasePath();
}

const char* Platform::GetPersistentPath() {
    return SDL_GetPrefPath("PrismaEngine", "");
}

const char* Platform::GetTemporaryPath() {
    static char tempPath[256] = {0};
    if (tempPath[0] == '\0') {
        strcpy(tempPath, "/tmp");
    }
    return tempPath;
}

// ------------------------------------------------------------
// 线程和同步
// ------------------------------------------------------------
PlatformThreadHandle Platform::CreateThread(ThreadFunc entry, void* userData) {
    return SDL_CreateThread((SDL_ThreadFunction)entry, "Prisma_Thread", userData);
}

void Platform::JoinThread(PlatformThreadHandle thread) {
    SDL_WaitThread(static_cast<SDL_Thread*>(thread), NULL);
}

PlatformMutexHandle Platform::CreateMutex() {
    return SDL_CreateMutex();
}

void Platform::DestroyMutex(PlatformMutexHandle mtx) {
    SDL_DestroyMutex(static_cast<SDL_Mutex*>(mtx));
}

void Platform::LockMutex(PlatformMutexHandle mtx) {
    SDL_LockMutex(static_cast<SDL_Mutex*>(mtx));
}

void Platform::UnlockMutex(PlatformMutexHandle mtx) {
    SDL_UnlockMutex(static_cast<SDL_Mutex*>(mtx));
}

// ------------------------------------------------------------
// Vulkan 支持
// ------------------------------------------------------------
std::vector<const char*> Platform::GetVulkanInstanceExtensions() {
    uint32_t count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!extensions) return {};
    std::vector<const char*> result;
    for (uint32_t i = 0; i < count; i++) {
        result.push_back(extensions[i]);
    }
    return result;
}

bool Platform::CreateVulkanSurface(void* instance, WindowHandle windowHandle, void** outSurface) {
    if (!instance || !windowHandle || !outSurface) return false;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (SDL_Vulkan_CreateSurface(static_cast<SDL_Window*>(windowHandle),
                                  static_cast<VkInstance>(instance), nullptr, &surface)) {
        *outSurface = (void*)surface;
        return true;
    }
    return false;
}

// ------------------------------------------------------------
// IPlatformLogger 接口实现
// ------------------------------------------------------------
void Platform::LogToConsole(PlatformLogLevel level, const char* tag, const char* message) {
    (void)level;
    (void)tag;
    std::cout << message << std::endl;
}

const char* Platform::GetLogDirectoryPath() {
    static char logPath[512] = {0};
    static bool initialized = false;

    if (!initialized) {
        const char* prefPath = SDL_GetPrefPath("PrismaEngine", "logs");
        if (prefPath) {
            strncpy(logPath, prefPath, sizeof(logPath) - 1);
            SDL_free(reinterpret_cast<void*>(const_cast<char*>(prefPath)));
        } else {
            strcpy(logPath, "logs");
        }
        initialized = true;
    }

    return logPath;
}

} // namespace Engine

#endif // PRISMA_SDL_AVAILABLE

#endif // !defined(_WIN32) && !defined(__ANDROID__)
