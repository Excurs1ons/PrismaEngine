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
#include <memory>
#include <cstdlib>

#ifdef _WIN32
    #include <process.h>
#else
    #include <unistd.h>
#endif
namespace Prisma {

namespace {
struct ThreadStartContext {
    ThreadFunc entry;
    void* userData;
};

int SDLThreadEntryPoint(void* rawContext) {
    std::unique_ptr<ThreadStartContext> context(static_cast<ThreadStartContext*>(rawContext));
    if (!context || !context->entry) {
        return -1;
    }

    context->entry(context->userData);
    return 0;
}

std::mutex& LocalTimeMutex() {
    static std::mutex mutex;
    return mutex;
}
} // namespace

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
    SDL_Log("%s", message ? message : "");
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
#ifdef _WIN32
    return (uint32_t)_getpid();
#else
    return (uint32_t)getpid();
#endif
}

std::tm Platform::GetLocalTime(std::time_t time) {
    std::tm tm{};
    std::lock_guard<std::mutex> lock(LocalTimeMutex());
    if (const std::tm* local = std::localtime(&time)) {
        tm = *local;
    }
    return tm;
}

void Platform::ShowMessageBox(const std::string& title, const std::string& message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), nullptr);
}

bool Platform::HasDisplaySupport() {
    if (SDL_GetCurrentVideoDriver() == nullptr) {
        return false;
    }

    int displayCount = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
    if (displays) {
        SDL_free(displays);
    }
    return displayCount > 0;
}

bool Platform::IsRunningInTerminal() {
    // SDL does not expose a direct tty query; keep behavior predictable in prune branch.
    return true;
}

std::string Platform::GetEnvironmentVariable(const std::string& name) {
    const char* val = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), name.c_str());
    return val ? std::string(val) : "";
}

void Platform::SetEnvironmentVariable(const std::string& name, const std::string& value) {
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), name.c_str(), value.c_str(), true);
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
// 文件系统 (优先使用 SDL3 API)
// ------------------------------------------------------------
bool Platform::FileExists(const char* path) {
    SDL_PathInfo info;
    return SDL_GetPathInfo(path, &info);
}

size_t Platform::FileSize(const char* path) {
    SDL_PathInfo info;
    if (SDL_GetPathInfo(path, &info)) {
        return (size_t)info.size;
    }
    return 0;
}

size_t Platform::ReadFile(const char* path, void* dst, size_t maxBytes) {
    size_t size = 0;
    void* data = SDL_LoadFile(path, &size);
    if (!data) return 0;

    size_t toCopy = std::min(size, maxBytes);
    std::memcpy(dst, data, toCopy);
    SDL_free(data);
    return toCopy;
}

bool Platform::SetCurrentDirectory(const char* path) {
    if (!path) return false;
    try {
        std::filesystem::current_path(path);
        return true;
    } catch (...) {
        return false;
    }
}

const char* Platform::GetExecutablePath() {
    static std::string path;
    const char* basePath = SDL_GetBasePath();
    if (basePath && *basePath) {
        path = basePath;
    }
    if (path.empty()) {
        path = ".";
    }
    return path.c_str();
}

const char* Platform::GetPersistentPath() {
    static std::string path;
    char* prefPath = SDL_GetPrefPath("Prisma", "PrismaEngine");
    if (prefPath && *prefPath) {
        path = prefPath;
        SDL_free(prefPath);
    } else {
        path = ".";
    }
    std::filesystem::create_directories(path);
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
    auto* context = new ThreadStartContext{entry, userData};
    SDL_Thread* thread = SDL_CreateThread(SDLThreadEntryPoint, "PrismaThread", context);
    if (!thread) {
        delete context;
        return nullptr;
    }
    return (PlatformThreadHandle)thread;
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
