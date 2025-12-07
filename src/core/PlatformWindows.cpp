#ifdef _WIN32
#include "PlatformWindows.h"
#include <Logger.h>
#include <Windows.h>
#include <chrono>
#include <codecvt>
#include <filesystem>
#include <iostream>
#include <locale>
#include <shlobj.h>
#include <thread>

#undef CreateWindow
#undef CreateMutex

namespace Engine {
static LARGE_INTEGER s_frequency;
static bool s_use_qpc = false;

PlatformWindows::PlatformWindows() {
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            LOG_INFO("Platform", "窗口即将关闭");
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            LOG_INFO("Platform", "窗口已关闭");
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}
WindowHandle PlatformWindows::CreateWindow(const WindowProps& props) {
    // 注册窗口类
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXA wc   = {};
        wc.cbSize        = sizeof(WNDCLASSEXA);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = GetModuleHandle(nullptr);
        wc.hIcon         = nullptr;
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName  = nullptr;
        wc.lpszClassName = "YAGEWindowClass";
        wc.hIconSm       = nullptr;

        if (!RegisterClassExA(&wc)) {
            LOG_ERROR("Platform", "注册窗口类失败");
            return nullptr;
        }
        classRegistered = true;
    }
    
    // 确定窗口样式
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!props.Resizable) {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    
    // 计算窗口大小（包括边框）
    RECT rect = {0, 0, static_cast<LONG>(props.Width), static_cast<LONG>(props.Height)};
    AdjustWindowRect(&rect, style, FALSE);
    
    // 创建窗口
    HWND hwnd = CreateWindowExA(
        0,                              // 扩展窗口样式
        "YAGEWindowClass",              // 窗口类名
        props.Title.c_str(),            // 窗口标题
        style,                          // 窗口样式
        CW_USEDEFAULT,                  // X 位置
        CW_USEDEFAULT,                  // Y 位置
        rect.right - rect.left,         // 宽度
        rect.bottom - rect.top,         // 高度
        nullptr,                        // 父窗口
        nullptr,                        // 菜单
        GetModuleHandle(nullptr),       // 实例句柄
        nullptr                         // 附加数据
    );
    
    if (!hwnd) {
        LOG_ERROR("Platform", "创建窗口失败");
        return nullptr;
    }
    
    // 显示窗口
    int showCmd = SW_SHOW;
    switch (props.ShowState) {
        case WindowShowState::Show:
            showCmd = SW_SHOW;
            break;
        case WindowShowState::Hide:
            showCmd = SW_HIDE;
            break;
        case WindowShowState::Maximize:
            showCmd = SW_MAXIMIZE;
            break;
        case WindowShowState::Minimize:
            showCmd = SW_MINIMIZE;
            break;
        default:
            showCmd = SW_SHOW;
            break;
    }
    
    ShowWindow(hwnd, showCmd);
    UpdateWindow(hwnd);
    
    // 保存窗口句柄
    this->hwnd = hwnd;
    
    LOG_INFO("Platform", "创建窗口成功: {0}", props.Title);
    return static_cast<WindowHandle>(hwnd);
}

bool PlatformWindows::SetWindowIcon(const std::string& path) {
    std::string finalPath = path + ".ico";  // Windows 固定 ico
    HICON hIcon           = (HICON)LoadImageA(nullptr, finalPath.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        return true;
    }
    return false;
}


void PlatformWindows::DestroyWindow(WindowHandle window) {
    // 销毁窗口的实现
    if (window) {
        ::DestroyWindow(static_cast<HWND>(window));
    }
}

void PlatformWindows::GetWindowSize(WindowHandle window, int& outW, int& outH) {
    // 获取窗口尺寸的实现
    if (window) {
        RECT rect;
        if (GetWindowRect(static_cast<HWND>(window), &rect)) {
            outW = rect.right - rect.left;
            outH = rect.bottom - rect.top;
        }
    }
}

void PlatformWindows::SetWindowTitle(WindowHandle window, const char* title) {
    // 设置窗口标题的实现
    if (window && title) {
        SetWindowTextA(static_cast<HWND>(window), title);
    }
}

bool PlatformWindows::ShouldClose(WindowHandle window) const {
    // 检查窗口是否应该关闭
    if (window) {
        MSG msg;
        // 检查是否有 WM_QUIT 消息
        if (PeekMessage(&msg, static_cast<HWND>(window), 0, 0, PM_NOREMOVE)) {
            if (msg.message == WM_QUIT) {
                return true;
            }
        }
    }
    return false;
}

uint64_t PlatformWindows::GetTimeMicroseconds() const {
    if (s_use_qpc) {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return static_cast<uint64_t>((counter.QuadPart * 1000000) / s_frequency.QuadPart);
    } else {
        return static_cast<uint64_t>(GetTickCount64() * 1000);
    }
}

double PlatformWindows::GetTimeSeconds() const {
    // 获取秒时间
    return GetTimeMicroseconds() / 1000000.0;
}

bool PlatformWindows::IsMouseButtonDown(MouseButton btn) const {
    // 检查鼠标按键状态
    SHORT state = GetAsyncKeyState(btn);
    return (state & 0x8000) != 0;
}

void PlatformWindows::GetMousePosition(float& x, float& y) const {
    // 获取鼠标位置
    POINT point;
    if (GetCursorPos(&point)) {
        x = static_cast<float>(point.x);
        y = static_cast<float>(point.y);
    } else {
        x = 0.0f;
        y = 0.0f;
    }
}

void PlatformWindows::SetMousePosition(float x, float y) {
    // 设置鼠标位置
    SetCursorPos(static_cast<int>(x), static_cast<int>(y));
}

void PlatformWindows::SetMouseLock(bool locked) {
    // 设置鼠标锁定状态
    if (locked) {
        // 锁定鼠标到当前窗口
        RECT rect;
        GetClientRect(hwnd, &rect);
        ClientToScreen(hwnd, reinterpret_cast<POINT*>(&rect.left));
        ClientToScreen(hwnd, reinterpret_cast<POINT*>(&rect.right));
        ClipCursor(&rect);
    } else {
        ClipCursor(nullptr);
    }
}

bool PlatformWindows::FileExists(const char* path) const {
    // 检查文件是否存在
    if (!path)
        return false;
    return std::filesystem::exists(path);
}

size_t PlatformWindows::FileSize(const char* path) const {
    // 获取文件大小
    if (!path)
        return 0;
    if (!std::filesystem::exists(path)) {
        return 0;
    }
    return std::filesystem::file_size(path);
}

size_t PlatformWindows::ReadFile(const char* path, void* dst, size_t maxBytes) const {
    // 读取文件内容
    if (!path || !dst) {
        return 0;
    }

    FILE* file = nullptr;
    fopen_s(&file, path, "rb");
    if (!file) {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    size_t bytesToRead = (fileSize < static_cast<long>(maxBytes)) ? fileSize : maxBytes;
    size_t bytesRead   = fread(dst, 1, bytesToRead, file);
    fclose(file);
    return bytesRead;
}

const char* PlatformWindows::GetExecutablePath() const {
    // 获取可执行文件路径
    static char path[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return path;
}

const char* PlatformWindows::GetPersistentPath() const {
    // 获取持久化存储路径
    static char path[MAX_PATH] = {0};
    // 使用 AppData/Roaming 目录
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path))) {
        return path;
    }
    return nullptr;
}

const char* PlatformWindows::GetTemporaryPath() const {
    // 获取临时文件路径
    static char path[MAX_PATH] = {0};
    GetTempPathA(MAX_PATH, path);
    return path;
}

PlatformThreadHandle PlatformWindows::CreateThread(ThreadFunc entry, void* userData) {
    // 创建线程
    DWORD threadId;
    HANDLE thread = ::CreateThread(
        nullptr,           // 默认安全属性
        0,                 // 默认堆栈大小
        reinterpret_cast<LPTHREAD_START_ROUTINE>(entry),
        userData,          // 参数
        0,                 // 立即运行
        &threadId          // 线程ID
    );
    return static_cast<PlatformThreadHandle>(thread);
}

void PlatformWindows::JoinThread(PlatformThreadHandle thread) {
    // 等待线程结束
    if (thread) {
        WaitForSingleObject(static_cast<HANDLE>(thread), INFINITE);
    }
}

PlatformMutexHandle PlatformWindows::CreateMutex() {
    // 创建互斥锁
    HANDLE mutex = CreateMutexA(nullptr, FALSE, nullptr);
    return static_cast<PlatformMutexHandle>(mutex);
}

void PlatformWindows::DestroyMutex(PlatformMutexHandle mtx) {
    // 销毁互斥锁
    if (mtx) {
        CloseHandle(static_cast<HANDLE>(mtx));
    }
}

void PlatformWindows::LockMutex(PlatformMutexHandle mtx) {
    // 锁定互斥锁
    if (mtx) {
        WaitForSingleObject(static_cast<HANDLE>(mtx), INFINITE);
    }
}

void PlatformWindows::UnlockMutex(PlatformMutexHandle mtx) {
    // 解锁互斥锁
    if (mtx) {
        ReleaseMutex(static_cast<HANDLE>(mtx));
    }
}

void PlatformWindows::SleepMilliseconds(uint32_t ms) {
    // 睡眠指定毫秒数
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool PlatformWindows::Initialize() {
    // 初始化平台
    s_use_qpc = QueryPerformanceFrequency(&s_frequency) != 0;

    return true;
}

void PlatformWindows::Shutdown() {
    // 关闭平台
}

bool PlatformWindows::IsKeyDown(KeyCode key) const {
    // 检查键盘按键状态
    SHORT state = GetAsyncKeyState(static_cast<int>(key));
    return (state & 0x8000) != 0;
}

void PlatformWindows::PumpEvents() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
}  // namespace Engine

#endif // _WIN32
