#pragma once

// Windows应用程序预编译头文件
// 包含Windows应用程序常用的头文件

// Windows特定头文件
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>

// 引擎和游戏头文件
#include "../../../src/engine/pch.h"
#include "../../../src/game/pch.h"

// Win32应用程序特定的头文件
#include <mmsystem.h>
#include <wincodec.h>
#include <d2d1.h>
#include <dwrite.h>

// 便利宏
#define WINDOW_CLASS_NAME L"PrismaEngineWindow"
#define APP_NAME L"Prisma Engine"

// 窗口消息处理宏
#define HANDLE_MSG(hwnd, message, fn) \
    case (message): return HANDLE_##message((hwnd), (wParam), (lParam), (fn))

// 常用的Windows资源ID
enum {
    ID_FILE_NEW = 1001,
    ID_FILE_OPEN,
    ID_FILE_SAVE,
    ID_FILE_EXIT,
    ID_EDIT_UNDO,
    ID_EDIT_REDO,
    ID_EDIT_CUT,
    ID_EDIT_COPY,
    ID_EDIT_PASTE,
    ID_VIEW_FULLSCREEN,
    ID_VIEW_SETTINGS,
    ID_HELP_ABOUT
};

// 便利的字符串转换
inline std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

inline std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}