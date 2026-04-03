#include "Platform.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <iostream>
#include <vector>
#include <filesystem>
#include <ctime>

namespace Prisma {

bool Platform::s_initialized     = false;
bool Platform::s_shouldClose     = false;
WindowHandle Platform::s_currentWindow = nullptr;
Platform::EventCallback Platform::s_eventCallback = nullptr;

bool Platform::Initialize() {
    if (s_initialized) return true;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    s_initialized = true;
    return true;
}

void Platform::Shutdown() {
    if (s_initialized) {
        SDL_Quit();
        s_initialized = false;
    }
}

bool Platform::IsInitialized() {
    return s_initialized;
}

void Platform::DebugPrint(const char* message) {
    SDL_Log("%s", message);
}

void Platform::SetConsoleColor(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   std::cout << "\033[90m"; break; // Gray
        case LogLevel::Debug:   std::cout << "\033[36m"; break; // Cyan
        case LogLevel::Info:    std::cout << "\033[32m"; break; // Green
        case LogLevel::Warning: std::cout << "\033[33m"; break; // Yellow
        case LogLevel::Error:   std::cout << "\033[31m"; break; // Red
        case LogLevel::Fatal:   std::cout << "\033[41m\033[37m"; break; // White on Red
        default: break;
    }
}

void Platform::ResetConsoleColor() {
    std::cout << "\033[0m";
}

uint32_t Platform::GetProcessId() {
    return 0; 
}

std::tm Platform::GetLocalTime(std::time_t time) {
    std::tm tm_struct;
#ifdef _WIN32
    localtime_s(&tm_struct, &time);
#else
    localtime_r(&time, &tm_struct);
#endif
    return tm_struct;
}

void Platform::ShowMessageBox(const std::string& title, const std::string& message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), nullptr);
}

bool Platform::HasDisplaySupport() {
    return SDL_WasInit(SDL_INIT_VIDEO) != 0;
}

bool Platform::IsRunningInTerminal() {
    return true; 
}

std::string Platform::GetEnvironmentVariable(const std::string& name) {
    const char* val = SDL_getenv(name.c_str());
    return val ? std::string(val) : "";
}

void Platform::SetEnvironmentVariable(const std::string& name, const std::string& value) {
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), name.c_str(), value.c_str(), 1);
}

// ------------------------------------------------------------
// Vulkan 支持
// ------------------------------------------------------------
std::vector<const char*> Platform::GetRequiredVulkanInstanceExtensions() {
    uint32_t count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!extensions) return {};
    
    return std::vector<const char*>(extensions, extensions + count);
}

bool Platform::CreateVulkanSurface(void* instance, WindowHandle window, void** outSurface) {
    if (!instance || !window || !outSurface) return false;
    
    VkSurfaceKHR surface;
    if (SDL_Vulkan_CreateSurface((SDL_Window*)window, (VkInstance)instance, nullptr, &surface)) {
        *outSurface = (void*)surface;
        return true;
    }
    return false;
}

// ------------------------------------------------------------
// 窗口管理
// ------------------------------------------------------------
WindowHandle Platform::CreateWindow(const WindowProps& desc) {
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    
    if (desc.Resizable) window_flags |= SDL_WINDOW_RESIZABLE;
    
    if (desc.fullScreenMode == FullScreenMode::FullScreen) window_flags |= SDL_WINDOW_FULLSCREEN;
    else if (desc.fullScreenMode == FullScreenMode::ExclusiveFullScreen) window_flags |= SDL_WINDOW_FULLSCREEN;

    if (desc.ShowState == WindowShowState::Hide) window_flags |= SDL_WINDOW_HIDDEN;
    else if (desc.ShowState == WindowShowState::Maximize) window_flags |= SDL_WINDOW_MAXIMIZED;
    else if (desc.ShowState == WindowShowState::Minimize) window_flags |= SDL_WINDOW_MINIMIZED;

    SDL_Window* window = SDL_CreateWindow(desc.Title.c_str(), desc.Width, desc.Height, window_flags);
    s_currentWindow = (WindowHandle)window;
    s_shouldClose = false;
    return s_currentWindow;
}

void Platform::DestroyWindow(WindowHandle window) {
    if (window) {
        SDL_DestroyWindow((SDL_Window*)window);
        if (s_currentWindow == window) s_currentWindow = nullptr;
    }
}

void Platform::GetWindowSize(WindowHandle window, int& outW, int& outH) {
    if (window) {
        SDL_GetWindowSize((SDL_Window*)window, &outW, &outH);
    } else {
        outW = 0; outH = 0;
    }
}

void Platform::SetWindowTitle(WindowHandle window, const char* title) {
    if (window) {
        SDL_SetWindowTitle((SDL_Window*)window, title);
    }
}

void Platform::PumpEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            s_shouldClose = true;
        }
    }
}

bool Platform::ShouldClose(WindowHandle window) {
    return s_shouldClose;
}

void Platform::SetShouldClose(WindowHandle window, bool shouldClose) {
    s_shouldClose = shouldClose;
}

WindowHandle Platform::GetCurrentWindow() {
    return s_currentWindow;
}

// ------------------------------------------------------------
// 时间管理
// ------------------------------------------------------------
uint64_t Platform::GetTimeMicroseconds() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

double Platform::GetTimeSeconds() {
    return GetTimeMicroseconds() / 1000000.0;
}

// ------------------------------------------------------------
// 文件系统
// ------------------------------------------------------------
bool Platform::FileExists(const char* path) {
    return std::filesystem::exists(path) && !std::filesystem::is_directory(path);
}

size_t Platform::FileSize(const char* path) {
    if (!std::filesystem::exists(path)) return 0;
    return (size_t)std::filesystem::file_size(path);
}

size_t Platform::ReadFile(const char* path, void* dst, size_t maxBytes) {
    FILE* file = fopen(path, "rb");
    if (!file) return 0;
    
    size_t read = fread(dst, 1, maxBytes, file);
    fclose(file);
    return read;
}

const char* Platform::GetExecutablePath() {
    static std::string path;
    const char* base_path_ptr = SDL_GetBasePath();
    if (base_path_ptr) {
        path = base_path_ptr;
        SDL_free(const_cast<char*>(base_path_ptr));
    }
    return path.c_str();
}

const char* Platform::GetPersistentPath() {
    static std::string path;
    const char* pref_path_ptr = SDL_GetPrefPath("Prisma", "Engine");
    if (pref_path_ptr) {
        path = pref_path_ptr;
        SDL_free(const_cast<char*>(pref_path_ptr));
    }
    return path.c_str();
}

const char* Platform::GetTemporaryPath() {
    static std::string path = std::filesystem::temp_directory_path().string();
    return path.c_str();
}

// ------------------------------------------------------------
// 线程和同步 (使用 SDL3 API)
// ------------------------------------------------------------
PlatformThreadHandle Platform::CreateThread(ThreadFunc entry, void* userData) {
    return (PlatformThreadHandle)SDL_CreateThread((SDL_ThreadFunction)entry, "PrismaThread", userData);
}

void Platform::JoinThread(PlatformThreadHandle thread) {
    if (thread) {
        SDL_WaitThread((SDL_Thread*)thread, nullptr);
    }
}

PlatformMutexHandle Platform::CreateMutex() {
    return (PlatformMutexHandle)SDL_CreateMutex();
}

void Platform::DestroyMutex(PlatformMutexHandle mtx) {
    if (mtx) SDL_DestroyMutex((SDL_Mutex*)mtx);
}

void Platform::LockMutex(PlatformMutexHandle mtx) {
    if (mtx) SDL_LockMutex((SDL_Mutex*)mtx);
}

void Platform::UnlockMutex(PlatformMutexHandle mtx) {
    if (mtx) SDL_UnlockMutex((SDL_Mutex*)mtx);
}

void Platform::SleepMilliseconds(uint32_t ms) {
    SDL_Delay(ms);
}

// ------------------------------------------------------------
// IPlatformLogger 接口实现
// ------------------------------------------------------------
void Platform::LogToConsole(LogLevel level, const char* tag, const char* message) {
    SetConsoleColor(level);
    std::cout << "[" << tag << "] " << message << std::endl;
    ResetConsoleColor();
}

const char* Platform::GetLogDirectoryPath() {
    static std::string path = std::string(GetPersistentPath()) + "/Logs";
    std::filesystem::create_directories(path);
    return path.c_str();
}

void Platform::SetEventCallback(EventCallback callback) {
    s_eventCallback = callback;
}

} // namespace Prisma
