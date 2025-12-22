#ifdef _WIN32
#include "PlatformWindows.h"
#include <Logger.h>
#define NOMINMAX
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

// 全局键盘状态数组
static bool g_keyStates[256] = { false };

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
        case WM_KEYDOWN:
            // 处理按键按下
            if (wParam < 256) {
                if (!g_keyStates[wParam]) {
                    g_keyStates[wParam] = true;
                    char keyChar = (wParam >= 'A' && wParam <= 'Z') ? static_cast<char>(wParam) : ' ';
                    LOG_INFO("Platform", "KeyDown: key={0} char='{1}'", static_cast<int>(wParam), keyChar);
                }
            }
            return 0;
        case WM_KEYUP:
            // 处理按键释放
            if (wParam < 256) {
                g_keyStates[wParam] = false;
                char keyChar = (wParam >= 'A' && wParam <= 'Z') ? static_cast<char>(wParam) : ' ';
                LOG_INFO("Platform", "KeyUp: key={0} char='{1}'", static_cast<int>(wParam), keyChar);
            }
            return 0;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

WindowHandle PlatformWindows::GetWindowHandle() const {
    return hwnd;
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
    return hwnd;
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
    if (hwnd) {
        hwnd = nullptr;
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
    // 如果窗口句柄无效，认为应该关闭
    if (!window) {
        return true;
    }

    // 检查窗口是否仍然存在
    if (!IsWindow(static_cast<HWND>(window))) {
        return true;
    }

    // 检查是否有 WM_QUIT 消息
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        if (msg.message == WM_QUIT) {
            return true;
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
    // 将 KeyCode 映射到 Windows 虚拟键码
    int virtualKey = 0;

    switch (key) {
        // 字母键
        case KeyCode::A: virtualKey = 'A'; break;
        case KeyCode::B: virtualKey = 'B'; break;
        case KeyCode::C: virtualKey = 'C'; break;
        case KeyCode::D: virtualKey = 'D'; break;
        case KeyCode::E: virtualKey = 'E'; break;
        case KeyCode::F: virtualKey = 'F'; break;
        case KeyCode::G: virtualKey = 'G'; break;
        case KeyCode::H: virtualKey = 'H'; break;
        case KeyCode::I: virtualKey = 'I'; break;
        case KeyCode::J: virtualKey = 'J'; break;
        case KeyCode::K: virtualKey = 'K'; break;
        case KeyCode::L: virtualKey = 'L'; break;
        case KeyCode::M: virtualKey = 'M'; break;
        case KeyCode::N: virtualKey = 'N'; break;
        case KeyCode::O: virtualKey = 'O'; break;
        case KeyCode::P: virtualKey = 'P'; break;
        case KeyCode::Q: virtualKey = 'Q'; break;
        case KeyCode::R: virtualKey = 'R'; break;
        case KeyCode::S: virtualKey = 'S'; break;
        case KeyCode::T: virtualKey = 'T'; break;
        case KeyCode::U: virtualKey = 'U'; break;
        case KeyCode::V: virtualKey = 'V'; break;
        case KeyCode::W: virtualKey = 'W'; break;
        case KeyCode::X: virtualKey = 'X'; break;
        case KeyCode::Y: virtualKey = 'Y'; break;
        case KeyCode::Z: virtualKey = 'Z'; break;

        // 数字键
        case KeyCode::Num0: virtualKey = '0'; break;
        case KeyCode::Num1: virtualKey = '1'; break;
        case KeyCode::Num2: virtualKey = '2'; break;
        case KeyCode::Num3: virtualKey = '3'; break;
        case KeyCode::Num4: virtualKey = '4'; break;
        case KeyCode::Num5: virtualKey = '5'; break;
        case KeyCode::Num6: virtualKey = '6'; break;
        case KeyCode::Num7: virtualKey = '7'; break;
        case KeyCode::Num8: virtualKey = '8'; break;
        case KeyCode::Num9: virtualKey = '9'; break;

        // 功能键
        case KeyCode::F1: virtualKey = VK_F1; break;
        case KeyCode::F2: virtualKey = VK_F2; break;
        case KeyCode::F3: virtualKey = VK_F3; break;
        case KeyCode::F4: virtualKey = VK_F4; break;
        case KeyCode::F5: virtualKey = VK_F5; break;
        case KeyCode::F6: virtualKey = VK_F6; break;
        case KeyCode::F7: virtualKey = VK_F7; break;
        case KeyCode::F8: virtualKey = VK_F8; break;
        case KeyCode::F9: virtualKey = VK_F9; break;
        case KeyCode::F10: virtualKey = VK_F10; break;
        case KeyCode::F11: virtualKey = VK_F11; break;
        case KeyCode::F12: virtualKey = VK_F12; break;

        // 方向键
        case KeyCode::ArrowUp: virtualKey = VK_UP; break;
        case KeyCode::ArrowDown: virtualKey = VK_DOWN; break;
        case KeyCode::ArrowLeft: virtualKey = VK_LEFT; break;
        case KeyCode::ArrowRight: virtualKey = VK_RIGHT; break;

        // 特殊键
        case KeyCode::Space: virtualKey = VK_SPACE; break;
        case KeyCode::Enter: virtualKey = VK_RETURN; break;
        case KeyCode::Escape: virtualKey = VK_ESCAPE; break;
        case KeyCode::Backspace: virtualKey = VK_BACK; break;
        case KeyCode::Tab: virtualKey = VK_TAB; break;
        case KeyCode::CapsLock: virtualKey = VK_CAPITAL; break;

        // 修饰键
        case KeyCode::LeftShift: virtualKey = VK_LSHIFT; break;
        case KeyCode::RightShift: virtualKey = VK_RSHIFT; break;
        case KeyCode::LeftControl: virtualKey = VK_LCONTROL; break;
        case KeyCode::RightControl: virtualKey = VK_RCONTROL; break;
        case KeyCode::LeftAlt: virtualKey = VK_LMENU; break;
        case KeyCode::RightAlt: virtualKey = VK_RMENU; break;

        // 符号键
        case KeyCode::Grave: virtualKey = VK_OEM_3; break;          // ` ~
        case KeyCode::Minus: virtualKey = VK_OEM_MINUS; break;    // - _
        case KeyCode::Equal: virtualKey = VK_OEM_PLUS; break;     // = +
        case KeyCode::LeftBracket: virtualKey = VK_OEM_4; break;  // [ {
        case KeyCode::RightBracket: virtualKey = VK_OEM_6; break; // ] }
        case KeyCode::Backslash: virtualKey = VK_OEM_5; break;    // \ |
        case KeyCode::Semicolon: virtualKey = VK_OEM_1; break;    // ; :
        case KeyCode::Apostrophe: virtualKey = VK_OEM_7; break;   // ' "
        case KeyCode::Comma: virtualKey = VK_OEM_COMMA; break;    // , <
        case KeyCode::Period: virtualKey = VK_OEM_PERIOD; break;  // . >
        case KeyCode::Slash: virtualKey = VK_OEM_2; break;        // / ?

        default: return false;
    }

    // 使用事件驱动的键盘状态
    bool pressed = g_keyStates[virtualKey];
    return pressed;
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
