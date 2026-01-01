#include "Platform.h"

#ifdef __ANDROID__

#include <android/log.h>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <pthread.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace PrismaEngine {

// ------------------------------------------------------------
// Android 平台静态变量
// ------------------------------------------------------------
static bool g_keyStates[256] = { false };

// ------------------------------------------------------------
// 平台生命周期管理
// ------------------------------------------------------------
bool Platform::Initialize() {
    if (s_initialized) {
        return true;
    }

    s_initialized = true;
    s_shouldClose = false;
    return true;
}

void Platform::Shutdown() {
    if (!s_initialized) {
        return;
    }

    s_initialized = false;
    s_currentWindow = nullptr;
}

// ------------------------------------------------------------
// 窗口管理 - Android 窗口由 native activity 管理
// ------------------------------------------------------------
WindowHandle Platform::CreateWindow(const WindowProps& desc) {
    // Android: 窗口由 NativeActivity 管理，这里返回 nullptr
    // 实际窗口通过 Android JNI 接口获取
    (void)desc;
    s_currentWindow = nullptr;
    return nullptr;
}

void Platform::DestroyWindow(WindowHandle window) {
    (void)window;
    s_currentWindow = nullptr;
}

void Platform::GetWindowSize(WindowHandle window, int& outW, int& outH) {
    // Android: 需要通过 JNI 获取窗口尺寸
    // 这里返回默认值
    (void)window;
    outW = 1280;
    outH = 720;
}

void Platform::SetWindowTitle(WindowHandle window, const char* title) {
    // Android: 不支持设置窗口标题
    (void)window;
    (void)title;
}

void Platform::PumpEvents() {
    // Android: 事件由 JNI 回调处理
}

bool Platform::ShouldClose(WindowHandle window) {
    (void)window;
    return s_shouldClose;
}

// ------------------------------------------------------------
// 时间管理
// ------------------------------------------------------------
uint64_t Platform::GetTimeMicroseconds() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

double Platform::GetTimeSeconds() {
    return GetTimeMicroseconds() / 1000000.0;
}

// ------------------------------------------------------------
// 输入管理 - Android 输入通过触摸事件处理
// ------------------------------------------------------------
bool Platform::IsKeyDown(Input::KeyCode key) {
    // Android: 键盘输入通常通过软键盘，这里返回 false
    (void)key;
    return false;
}

bool Platform::IsMouseButtonDown(Input::MouseButton btn) {
    // Android: 没有鼠标，返回 false
    (void)btn;
    return false;
}

void Platform::GetMousePosition(float& x, float& y) {
    // Android: 没有鼠标
    x = 0.0f;
    y = 0.0f;
}

void Platform::SetMousePosition(float x, float y) {
    // Android: 不支持
    (void)x;
    (void)y;
}

void Platform::SetMouseLock(bool locked) {
    // Android: 不支持
    (void)locked;
}

// ------------------------------------------------------------
// 文件系统
// ------------------------------------------------------------
bool Platform::FileExists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

size_t Platform::FileSize(const char* path) {
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        return 0;
    }
    return static_cast<size_t>(buffer.st_size);
}

size_t Platform::ReadFile(const char* path, void* dst, size_t maxBytes) {
    if (!path || !dst) {
        return 0;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    size_t bytesToRead = (fileSize < static_cast<long>(maxBytes)) ? fileSize : maxBytes;
    size_t bytesRead = fread(dst, 1, bytesToRead, file);
    fclose(file);
    return bytesRead;
}

const char* Platform::GetExecutablePath() {
    static char path[256] = {0};
    if (path[0] == '\0') {
        // Android: 获取应用私有目录
        const char* dataDir = getenv("ANDROID_DATA");
        if (dataDir) {
            strncpy(path, dataDir, sizeof(path) - 1);
        } else {
            strcpy(path, "/data/data/com.prisma.engine");
        }
    }
    return path;
}

const char* Platform::GetPersistentPath() {
    // Android: 内部存储路径
    // 通常通过 JNI 获取: context.getFilesDir()
    static char path[256] = {0};
    if (path[0] == '\0') {
        const char* filesDir = getenv("ANDROID_FILES_DIR");
        if (filesDir) {
            strncpy(path, filesDir, sizeof(path) - 1);
        } else {
            // 默认路径
            strcpy(path, "/data/data/com.prisma.engine/files");
        }
    }
    return path;
}

const char* Platform::GetTemporaryPath() {
    // Android: 使用缓存目录
    static char path[256] = {0};
    if (path[0] == '\0') {
        const char* cacheDir = getenv("ANDROID_CACHE_DIR");
        if (cacheDir) {
            strncpy(path, cacheDir, sizeof(path) - 1);
        } else {
            // 默认路径
            strcpy(path, "/data/data/com.prisma.engine/cache");
        }
    }
    return path;
}

// ------------------------------------------------------------
// 线程和同步
// ------------------------------------------------------------
PlatformThreadHandle Platform::CreateThread(ThreadFunc entry, void* userData) {
    pthread_t* thread = new pthread_t;
    int result = pthread_create(thread, nullptr, reinterpret_cast<void*(*)(void*)>(entry), userData);
    if (result != 0) {
        delete thread;
        return nullptr;
    }
    return thread;
}

void Platform::JoinThread(PlatformThreadHandle thread) {
    if (thread) {
        pthread_join(*reinterpret_cast<pthread_t*>(thread), nullptr);
        delete reinterpret_cast<pthread_t*>(thread);
    }
}

PlatformMutexHandle Platform::CreateMutex() {
    pthread_mutex_t* mutex = new pthread_mutex_t;
    pthread_mutex_init(mutex, nullptr);
    return mutex;
}

void Platform::DestroyMutex(PlatformMutexHandle mtx) {
    if (mtx) {
        pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(mtx));
        delete reinterpret_cast<pthread_mutex_t*>(mtx);
    }
}

void Platform::LockMutex(PlatformMutexHandle mtx) {
    if (mtx) {
        pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mtx));
    }
}

void Platform::UnlockMutex(PlatformMutexHandle mtx) {
    if (mtx) {
        pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mtx));
    }
}

// ------------------------------------------------------------
// IPlatformLogger 接口实现 - 使用 Android logcat
// ------------------------------------------------------------
void Platform::LogToConsole(LogLevel level, const char* tag, const char* message) {
    // 将 LogLevel 映射到 Android 日志优先级
    int priority;
    switch (level) {
        case LogLevel::Trace:
            priority = ANDROID_LOG_VERBOSE;
            break;
        case LogLevel::Debug:
            priority = ANDROID_LOG_DEBUG;
            break;
        case LogLevel::Info:
            priority = ANDROID_LOG_INFO;
            break;
        case LogLevel::Warning:
            priority = ANDROID_LOG_WARN;
            break;
        case LogLevel::Error:
            priority = ANDROID_LOG_ERROR;
            break;
        case LogLevel::Fatal:
            priority = ANDROID_LOG_FATAL;
            break;
        default:
            priority = ANDROID_LOG_INFO;
            break;
    }

    // 输出到 Android logcat
    __android_log_print(priority, tag ? tag : "PrismaEngine", "%s", message);
}

const char* Platform::GetLogDirectoryPath() {
    // Android 日志目录
    // 路径: /data/data/com.package.name/files/logs 或 cache/logs
    static char logPath[512] = {0};
    static bool initialized = false;

    if (!initialized) {
        const char* filesDir = getenv("ANDROID_FILES_DIR");
        if (filesDir) {
            snprintf(logPath, sizeof(logPath), "%s/logs", filesDir);
        } else {
            // 默认路径
            strcpy(logPath, "/data/data/com.prisma.engine/files/logs");
        }
        initialized = true;
    }

    return logPath;
}

} // namespace Engine

#endif // __ANDROID__
