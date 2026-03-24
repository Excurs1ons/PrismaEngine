#include "Platform.h"

#ifdef _WIN32
#include <Windows.h>
#include <process.h>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <shlobj.h>
#include <chrono>

// Undefine Win32 macros that conflict with our method names
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable
#undef CreateWindow
#undef CreateMutex
#undef DestroyWindow

namespace Prisma {

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
Platform::EventCallback Platform::s_eventCallback = nullptr;

namespace {
LRESULT CALLBACK PrismaWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            Prisma::Platform::SetShouldClose(hwnd, true);
            return 0;
        case WM_DESTROY:
            Prisma::Platform::SetShouldClose(hwnd, true);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}
} // namespace

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
    DWORD size = ::GetEnvironmentVariableA(name.c_str(), buffer, 1024);
    if (size > 0 && size < 1024) return std::string(buffer);
    return "";
}

void Platform::SetEnvironmentVariable(const std::string& name, const std::string& value) {
    ::SetEnvironmentVariableA(name.c_str(), value.c_str());
}

// 窗口管理函数
WindowHandle Platform::CreateWindow(const WindowProps& desc) {
    // 使用Windows API创建窗口，避免与类方法名冲突
    WNDCLASSA wc = {};
    wc.lpfnWndProc = PrismaWindowProc;
    wc.hInstance = ::GetModuleHandleA(nullptr);
    wc.lpszClassName = "PrismaEngine";
    wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    ::RegisterClassA(&wc);

    RECT windowRect = {0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height)};
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!desc.Resizable) {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    ::AdjustWindowRect(&windowRect, style, FALSE);
    
    HWND hwnd = ::CreateWindowExA(
        0,
        "PrismaEngine",
        desc.Title.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, nullptr,
        ::GetModuleHandleA(nullptr),
        nullptr
    );

    if (!hwnd) {
        return nullptr;
    }

    switch (desc.ShowState) {
        case WindowShowState::Hide:
            ::ShowWindow(hwnd, SW_HIDE);
            break;
        case WindowShowState::Maximize:
            ::ShowWindow(hwnd, SW_MAXIMIZE);
            break;
        case WindowShowState::Minimize:
            ::ShowWindow(hwnd, SW_MINIMIZE);
            break;
        case WindowShowState::Show:
        case WindowShowState::Default:
        default:
            ::ShowWindow(hwnd, SW_SHOWDEFAULT);
            break;
    }
    ::UpdateWindow(hwnd);

    s_currentWindow = (WindowHandle)hwnd;
    s_shouldClose = false;
    return s_currentWindow;
}

void Platform::DestroyWindow(WindowHandle window) {
    if (!window) {
        return;
    }

    ::DestroyWindow(static_cast<HWND>(window));
    if (s_currentWindow == window) {
        s_currentWindow = nullptr;
    }
}

void Platform::GetWindowSize(WindowHandle window, int& outW, int& outH) {
    if (!window) {
        outW = 0;
        outH = 0;
        return;
    }

    RECT rect{};
    if (::GetClientRect(static_cast<HWND>(window), &rect)) {
        outW = rect.right - rect.left;
        outH = rect.bottom - rect.top;
    } else {
        outW = 0;
        outH = 0;
    }
}

void Platform::SetWindowTitle(WindowHandle window, const char* title) {
    if (window && title) {
        ::SetWindowTextA(static_cast<HWND>(window), title);
    }
}

void Platform::PumpEvents() {
    // 处理Windows消息
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool Platform::ShouldClose(WindowHandle window) {
    if (window) {
        return !::IsWindow(static_cast<HWND>(window)) || s_shouldClose;
    }
    return s_shouldClose;
}

void Platform::SetShouldClose(WindowHandle window, bool shouldClose) {
    (void)window;
    s_shouldClose = shouldClose;
}

WindowHandle Platform::GetCurrentWindow() {
    return s_currentWindow;
}

// 时间管理函数
uint64_t Platform::GetTimeMicroseconds() {
    static LARGE_INTEGER frequency;
    static bool firstCall = true;
    
    if (firstCall) {
        QueryPerformanceFrequency(&frequency);
        firstCall = false;
    }
    
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    
    return (counter.QuadPart * 1000000) / frequency.QuadPart;
}

double Platform::GetTimeSeconds() {
    return GetTimeMicroseconds() / 1000000.0;
}

// 文件系统函数
bool Platform::FileExists(const char* path) {
    DWORD attrib = GetFileAttributesA(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

size_t Platform::FileSize(const char* path) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &fileInfo)) {
        return (size_t)fileInfo.nFileSizeLow | ((size_t)fileInfo.nFileSizeHigh << 32);
    }
    return 0;
}

size_t Platform::ReadFile(const char* path, void* dst, size_t maxBytes) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    
    DWORD bytesRead;
    DWORD toRead = (DWORD)(maxBytes > 0xFFFFFFFF ? 0xFFFFFFFF : maxBytes);
    BOOL result = ::ReadFile(hFile, dst, toRead, &bytesRead, nullptr);
    CloseHandle(hFile);
    
    return result ? (size_t)bytesRead : 0;
}

const char* Platform::GetExecutablePath() {
    static char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return buffer;
}

const char* Platform::GetPersistentPath() {
    static char buffer[MAX_PATH];
    if (SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, buffer) == S_OK) {
        return buffer;
    }
    return ".";
}

const char* Platform::GetTemporaryPath() {
    static char buffer[MAX_PATH];
    GetTempPathA(MAX_PATH, buffer);
    return buffer;
}

// 线程和同步函数
PlatformThreadHandle Platform::CreateThread(ThreadFunc entry, void* userData) {
    return (PlatformThreadHandle)_beginthreadex(nullptr, 0, (unsigned(__stdcall*)(void*))entry, userData, 0, nullptr);
}

void Platform::JoinThread(PlatformThreadHandle thread) {
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

PlatformMutexHandle Platform::CreateMutex() {
    return (PlatformMutexHandle)::CreateMutexA(nullptr, FALSE, nullptr);
}

void Platform::DestroyMutex(PlatformMutexHandle mtx) {
    ::CloseHandle(mtx);
}

void Platform::LockMutex(PlatformMutexHandle mtx) {
    ::WaitForSingleObject(mtx, INFINITE);
}

void Platform::UnlockMutex(PlatformMutexHandle mtx) {
    ::ReleaseMutex(mtx);
}

// IPlatformLogger接口实现
void Platform::LogToConsole(LogLevel level, const char* tag, const char* message) {
    // 简单实现 - 输出到stdout
    SetConsoleColor(level);
    std::cout << "[" << tag << "] " << message << std::endl;
    ResetConsoleColor();
}

const char* Platform::GetLogDirectoryPath() {
    static char buffer[MAX_PATH];
    if (SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, buffer) == S_OK) {
        strcat_s(buffer, "\\PrismaEngine\\Logs");
        CreateDirectoryA(buffer, nullptr);
        return buffer;
    }
    return ".";
}

// SDL特定功能
void Platform::SetEventCallback(EventCallback callback) {
    s_eventCallback = callback;
}

} // namespace Prisma
#endif
