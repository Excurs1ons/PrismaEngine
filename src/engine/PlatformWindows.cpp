#include "Platform.h"

#ifdef _WIN32
#include <Windows.h>
#include <process.h>
#include <iostream>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace Prisma {

...

std::vector<const char*> Platform::GetRequiredVulkanInstanceExtensions() {
    return { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
}

bool Platform::CreateVulkanSurface(void* instance, WindowHandle window, void** outSurface) {
    if (!instance || !window || !outSurface) return false;

    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = static_cast<HWND>(window);
    createInfo.hinstance = GetModuleHandle(nullptr);

    VkSurfaceKHR surface;
    if (vkCreateWin32SurfaceKHR(static_cast<VkInstance>(instance), &createInfo, nullptr, &surface) == VK_SUCCESS) {
        *outSurface = static_cast<void*>(surface);
        return true;
    }
    return false;
}

bool Platform::s_initialized = false;
bool Platform::s_shouldClose = false;
WindowHandle Platform::s_currentWindow = nullptr;

bool Platform::Initialize() {
    if (s_initialized) return true;
    s_initialized = true;
    return true;
}

void Platform::Shutdown() {
    s_initialized = false;
}

bool Platform::IsInitialized() {
    return s_initialized;
}

void Platform::DebugPrint(const char* message) {
    OutputDebugStringA(message);
}

void Platform::SetConsoleColor(LogLevel level) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;

    WORD attr = 0;
    switch (level) {
        case LogLevel::Trace:   attr = FOREGROUND_INTENSITY; break;
        case LogLevel::Debug:   attr = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case LogLevel::Info:    attr = FOREGROUND_GREEN; break;
        case LogLevel::Warning: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case LogLevel::Error:   attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        case LogLevel::Fatal:   attr = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        default:                attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }
    SetConsoleTextAttribute(hConsole, attr);
}

void Platform::ResetConsoleColor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}

uint32_t Platform::GetProcessId() {
    return (uint32_t)GetCurrentProcessId();
}

std::tm Platform::GetLocalTime(std::time_t time) {
    std::tm tm;
    localtime_s(&tm, &time);
    return tm;
}

void Platform::ShowMessageBox(const std::string& title, const std::string& message) {
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

bool Platform::HasDisplaySupport() {
    return GetSystemMetrics(SM_REMOTESESSION) == 0;
}

bool Platform::IsRunningInTerminal() {
    DWORD processCount;
    if (GetConsoleProcessList(&processCount, 1) == 0) return false;
    return processCount > 1;
}

std::string Platform::GetEnvironmentVariable(const std::string& name) {
    char buffer[1024];
    DWORD size = GetEnvironmentVariableA(name.c_str(), buffer, 1024);
    if (size > 0 && size < 1024) return std::string(buffer);
    return "";
}

void Platform::SetEnvironmentVariable(const std::string& name, const std::string& value) {
    SetEnvironmentVariableA(name.c_str(), value.c_str());
}

} // namespace Prisma
#endif
