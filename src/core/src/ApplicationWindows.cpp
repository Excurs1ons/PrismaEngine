//#include "ApplicationWindows.h"
//#include <iostream>
//#include "RenderBackendDirectX12.h"
//#include "resourcemanager.h"
//#include <memory>
//#include "logger.h"
//#include <windowsx.h>
//#include <dwmapi.h>
//#include <PlatformWindows.h>
//#include <windows.h>
//
//#include "AudioBackendXAudio2.h"
//#include "Scene.h"
//#include "MeshRenderer.h"
//#include "Transform.h"
//
//
//#include "Engine.h"
////#pragma comment(lib, "dwmapi.lib") // 告诉链接器链接这个库
//using namespace std;
//
//using Microsoft::WRL::ComPtr;
//// ============================================================================
//// 窗口类配置
//// ============================================================================
//constexpr int kMaxLoadStringLength = 100;
//
//// ============================================================================
//// 调试工具类
//// ============================================================================
//
//#if defined(_DEBUG)
///// @brief 将标准输出重定向到 Visual Studio 调试输出窗口
//class DebugOutputBuffer : public std::stringbuf {
//protected:
//    int sync() override {
//        OutputDebugStringA(str().c_str());
//        str(""); // 清空缓冲区
//        return 0;
//    }
//};
//
///// @brief 初始化调试输出重定向
///// @note 仅在 Debug 模式下有效
//inline void InitializeDebugOutput() {
//    static DebugOutputBuffer debugBuffer;
//    std::cout.rdbuf(&debugBuffer);
//}
//#endif
//
//
//
//bool isRunningInDebugger() {
//    return ::IsDebuggerPresent() != FALSE;
//}
//
///// @brief 加载并配置窗口类
///// @param hInstance 应用程序实例句柄
///// @param className 窗口类名称
///// @return 配置好的窗口类结构
//WNDCLASSEXW CreateWindowClass(HINSTANCE hInstance, const wchar_t* className) {
//
//    WNDCLASSEXW wcex = {};
//    wcex.cbSize = sizeof(WNDCLASSEXW);
//    wcex.style = CS_HREDRAW | CS_VREDRAW;
//    wcex.lpfnWndProc = ApplicationWindows::WndProc;
//    wcex.cbClsExtra = 0;
//    wcex.cbWndExtra = 0;
//    wcex.hInstance = hInstance;
//    wcex.hIcon = nullptr;//LoadIcon(hInstance, MAKEINTRESOURCE(IDC_GAME));
//    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
//    wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)); // 使用黑色背景
//    wcex.lpszMenuName = nullptr; // 移除菜单
//    wcex.lpszClassName = className;
//    wcex.hIconSm = nullptr; //LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
//
//    return wcex;
//}
//
///// @brief 加载字符串资源
///// @param hInstance 应用程序实例句柄
///// @param resourceId 资源ID
///// @return 加载的字符串
//std::wstring LoadResourceString(HINSTANCE hInstance, UINT resourceId) {
//    wchar_t buffer[kMaxLoadStringLength];
//    LoadStringW(hInstance, resourceId, buffer, kMaxLoadStringLength);
//    return std::wstring(buffer);
//}
//namespace Engine {
//    // 定义静态成员变量
//    bool waitingLogged = false;
//    std::unique_ptr<ApplicationWindows> ApplicationWindows::instance = nullptr;
//    ApplicationWindows::ApplicationWindows() {
//        LOG_INFO("Application", "正在创建");
//        LOG_INFO("Application", "正在调试器中运行: {0}", isRunningInDebugger());
//
//        HINSTANCE hInstance = GetModuleHandleW(0);
//        this->hInstance = hInstance;
//        LOG_INFO("Application", "已获取实例句柄");
//        std::wstring windowTitle = L"ApplicationWindows";
//        std::wstring windowClassName = L"ApplicationWindowsClass";
//
//        // 创建并注册窗口类
//        WNDCLASSEXW windowClass = CreateWindowClass(hInstance, windowClassName.c_str());
//        ATOM classAtom = RegisterClassExW(&windowClass);
//        if (!classAtom) {
//            DWORD error = GetLastError();
//            LOG_ERROR("Application", "Failed to register window class, error code: {0}", error);
//        }
//
//        size_t titleLen = (windowTitle.length() + 1) * sizeof(wchar_t);
//        size_t classLen = (windowClassName.length() + 1) * sizeof(wchar_t);
//
//        wchar_t* titleCopy = new wchar_t[titleLen / sizeof(wchar_t)];
//        wchar_t* classCopy = new wchar_t[classLen / sizeof(wchar_t)];
//
//        memcpy(titleCopy, windowTitle.c_str(), titleLen);
//        memcpy(classCopy, windowClassName.c_str(), classLen);
//
//        this->szTitle = titleCopy;
//        this->szWindowClass = classCopy;
//
//        m_width = 1600.0f;
//        m_height = 900.0f;
//        this->wcex = windowClass;
//
//        STARTUPINFO startupInfo;
//        GetStartupInfo(&startupInfo);
//        int nCmdShow = startupInfo.dwFlags & STARTF_USESHOWWINDOW ?
//            startupInfo.wShowWindow : SW_SHOWDEFAULT;
//
//        this->nCmdShow = nCmdShow;
//    }
//
//    ApplicationWindows::ApplicationWindows(LPCWSTR szWindowClass, LPCWSTR szTitle, int nCmdShow, WNDCLASSEXW wcex)
//    {
//        this->szTitle = szTitle;
//        this->szWindowClass = szWindowClass;
//        m_width = 1600.0f;
//        m_height = 900.0f;
//        this->wcex = wcex;
//        this->hInstance = wcex.hInstance;
//        this->nCmdShow = nCmdShow;
//        this->m_Platform = std::make_unique<PlatformWindows>();
//    }
//
//    bool ApplicationWindows::Initialize()
//    {
//        LOG_INFO("Application", "正在初始化");
//        auto& resourceManager = ResourceManager::GetInstance();
//        if (!resourceManager.Initialize()) {
//            LOG_ERROR("Resource", "资源管理器初始化失败");
//            return false;
//        }
//        LOG_INFO("Resource", "资源管理器已初始化");
//
//        if (!szWindowClass) {
//            LOG_ERROR("Application", "窗口类名为空");
//        }
//        if (!szTitle) {
//            LOG_ERROR("Application", "窗口标题为空");
//        }
//        if (!hInstance) {
//            LOG_ERROR("Application", "实例句柄为空");
//        }
//        HWND hWnd = CreateWindowExW(0L, szWindowClass, szTitle, WS_POPUP,
//            CW_USEDEFAULT, 0, static_cast<int>(m_width), static_cast<int>(m_height), nullptr, nullptr, hInstance, nullptr);
//
//        if (!hWnd)
//        {
//            DWORD error = GetLastError();
//            LOG_FATAL("Application", "创建窗口失败，错误代码: {0}, 错误信息: {1}", error, "请查看调试输出了解详细信息");
//            return false;
//        }
//        LOG_INFO("Application", "主窗口已创建");
//
//        ShowWindow(hWnd, nCmdShow);
//        UpdateWindow(hWnd);
//
//        WindowProps desc;
//        desc.Width           = 1600;
//        desc.Height          = 900;
//        desc.Title           = "YAGE Application";
//        desc.Resizable       = true;
//        desc.FullScreenMode  = FullScreenMode::Window;
//        desc.ShowState       = WindowShowState::Default;
//
//        auto engine = Engine();
//        LOG_INFO("Application", "初始化完成");
//        return true;
//    }
//
//    int ApplicationWindows::Run()
//    {
//        MSG msg = {};
//
//        // 主动渲染循环
//        while (true)
//        {
//            // 处理Windows消息
//            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//            {
//                if (msg.message == WM_QUIT)
//                {
//                    break;
//                }
//                TranslateMessage(&msg);
//                DispatchMessage(&msg);
//            }
//            else
//            {
//                if (!renderBackend) {
//                    LOG_WARN("Application", "等待渲染后端创建...");
//                    continue;
//                }
//                // 主动渲染帧
//                if (!renderBackend->isInitialized)
//                {
//                    LOG_WARN("Application", "等待渲染后端初始化...");
//                }
//                renderBackend->BeginFrame();
//                renderBackend->EndFrame();
//                renderBackend->Present();
//            }
//        }
//        return (int)msg.wParam;
//    }
//
//
//    void ApplicationWindows::Shutdown()
//    {
//    }
//
//}