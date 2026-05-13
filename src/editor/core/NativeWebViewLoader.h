#pragma once

#include <string>
#include <functional>

#if defined(_WIN32)
#include <windows.h>
typedef HMODULE LIB_HANDLE;
#else
#include <dlfcn.h>
typedef void* LIB_HANDLE;
#endif

namespace Prisma {

/**
 * @brief 运行时动态加载系统 Webview 内核
 * 
 * Windows: 加载 WebView2Loader.dll
 * Linux: 加载 libwebkit2gtk-4.1.so.0
 * macOS: 加载 WebKit.framework
 */
class NativeWebViewLoader {
public:
    static LIB_HANDLE LoadSystemLibrary() {
#if defined(_WIN32)
        // 尝试从系统路径加载 WebView2 引导程序
        return LoadLibraryA("WebView2Loader.dll");
#elif defined(__linux__)
        // 尝试加载 Linux 系统的 WebKitGTK
        LIB_HANDLE handle = dlopen("libwebkit2gtk-4.1.so.0", RTLD_LAZY);
        if (!handle) handle = dlopen("libwebkit2gtk-4.0.so.3", RTLD_LAZY);
        return handle;
#else
        return nullptr;
#endif
    }

    static void Unload(LIB_HANDLE handle) {
        if (!handle) return;
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
    }
};

} // namespace Prisma
