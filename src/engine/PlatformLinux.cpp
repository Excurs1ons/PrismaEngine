#include "Platform.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

namespace Prisma {

bool Platform::s_initialized = false;
bool Platform::s_shouldClose = false;
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
    std::cout << message;
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
    return (uint32_t)getpid();
}

std::tm Platform::GetLocalTime(std::time_t time) {
    std::tm tm;
    localtime_r(&time, &tm);
    return tm;
}

void Platform::ShowMessageBox(const std::string& title, const std::string& message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), nullptr);
}

bool Platform::HasDisplaySupport() {
    return true; // Assume true for now
}

bool Platform::IsRunningInTerminal() {
    return isatty(STDOUT_FILENO);
}

std::string Platform::GetEnvironmentVariable(const std::string& name) {
    const char* val = getenv(name.c_str());
    return val ? std::string(val) : "";
}

void Platform::SetEnvironmentVariable(const std::string& name, const std::string& value) {
    setenv(name.c_str(), value.c_str(), 1);
}

// Vulkan Support
std::vector<const char*> Platform::GetRequiredVulkanInstanceExtensions() {
    uint32_t count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!extensions) return {};
    
    std::vector<const char*> result(extensions, extensions + count);
    return result;
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

// Window Management
WindowHandle Platform::CreateWindow(const WindowProps& desc) {
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Window* window = SDL_CreateWindow(desc.Title.c_str(), desc.Width, desc.Height, window_flags);
    s_currentWindow = (WindowHandle)window;
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

// Time Management
uint64_t Platform::GetTimeMicroseconds() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

double Platform::GetTimeSeconds() {
    return GetTimeMicroseconds() / 1000000.0;
}

// File System
bool Platform::FileExists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0 && S_ISREG(buffer.st_mode));
}

size_t Platform::FileSize(const char* path) {
    struct stat buffer;
    if (stat(path, &buffer) == 0) return (size_t)buffer.st_size;
    return 0;
}

size_t Platform::ReadFile(const char* path, void* dst, size_t maxBytes) {
    FILE* file = fopen(path, "rb");
    if (!file) return 0;
    
    size_t read = fread(dst, 1, maxBytes, file);
    fclose(file);
    return read;
}

const char* Platform::GetExecutablePath() {
    static char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return buffer;
    }
    return "";
}

const char* Platform::GetPersistentPath() {
    static std::string path;
    const char* home = getenv("HOME");
    if (home) {
        path = std::string(home) + "/.local/share/PrismaEngine";
        mkdir(path.c_str(), 0755);
        return path.c_str();
    }
    return ".";
}

const char* Platform::GetTemporaryPath() {
    return "/tmp";
}

// Threads and Synchronization
PlatformThreadHandle Platform::CreateThread(ThreadFunc entry, void* userData) {
    std::thread* t = new std::thread(entry, userData);
    return (PlatformThreadHandle)t;
}

void Platform::JoinThread(PlatformThreadHandle thread) {
    std::thread* t = (std::thread*)thread;
    if (t->joinable()) t->join();
    delete t;
}

PlatformMutexHandle Platform::CreateMutex() {
    return (PlatformMutexHandle)new std::mutex();
}

void Platform::DestroyMutex(PlatformMutexHandle mtx) {
    delete (std::mutex*)mtx;
}

void Platform::LockMutex(PlatformMutexHandle mtx) {
    ((std::mutex*)mtx)->lock();
}

void Platform::UnlockMutex(PlatformMutexHandle mtx) {
    ((std::mutex*)mtx)->unlock();
}

// IPlatformLogger
void Platform::LogToConsole(LogLevel level, const char* tag, const char* message) {
    SetConsoleColor(level);
    std::cout << "[" << tag << "] " << message << std::endl;
    ResetConsoleColor();
}

const char* Platform::GetLogDirectoryPath() {
    static std::string path = std::string(GetPersistentPath()) + "/Logs";
    mkdir(path.c_str(), 0755);
    return path.c_str();
}

void Platform::SetEventCallback(EventCallback callback) {
    s_eventCallback = callback;
}

} // namespace Prisma
