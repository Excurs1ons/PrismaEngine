#include "Platform.h"

#include <chrono>
#include <iostream>
#include <thread>

// 条件包含平台特定头文件
#ifdef _WIN32
    #include <Windows.h>
#endif

#if !defined(_WIN32) && !defined(__ANDROID__)
    #if defined(PRISMA_HAS_SDL) || defined(__has_include)
        #ifdef __has_include
            #if __has_include(<SDL3/SDL.h>)
                #include <SDL3/SDL.h>
            #endif
        #endif
    #endif
#endif

// ------------------------------------------------------------
// Time 类实现 - Time 定义在全局命名空间
// ------------------------------------------------------------
float Time::DeltaTime = 0.0f;
float Time::TotalTime = 0.0f;
float Time::TimeScale = 1.0f;

float Time::GetTime() {
    using namespace std::chrono;
    static auto start = high_resolution_clock::now();
    auto now = high_resolution_clock::now();
    return duration<float>(now - start).count();
}

namespace Engine {

// ------------------------------------------------------------
// 静态变量定义
// ------------------------------------------------------------
static bool s_initialized = false;
static bool s_shouldClose = false;
static WindowHandle s_currentWindow = nullptr;

#if !defined(_WIN32) && !defined(__ANDROID__)
static Platform::EventCallback s_eventCallback = nullptr;
#endif

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

bool Platform::IsInitialized() {
    return s_initialized;
}

// ------------------------------------------------------------
// 窗口管理
// ------------------------------------------------------------
WindowHandle Platform::GetCurrentWindow() {
    return s_currentWindow;
}

// ------------------------------------------------------------
// 时间管理（通用实现，平台可覆盖）
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
// 线程和同步（通用实现）
// ------------------------------------------------------------
void Platform::SleepMilliseconds(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ------------------------------------------------------------
// 文件系统（通用实现）
// ------------------------------------------------------------
#if !defined(_WIN32) && !defined(__ANDROID__)
    // Linux/其他平台的通用实现
    #include <sys/stat.h>
    #include <unistd.h>

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
            ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if (count > 0) {
                path[count] = '\0';
            } else {
                strcpy(path, ".");
            }
        }
        return path;
    }

    const char* Platform::GetPersistentPath() {
        static char path[256] = {0};
        if (path[0] == '\0') {
            const char* home = getenv("HOME");
            if (home) {
                snprintf(path, sizeof(path), "%s/.local/share/PrismaEngine", home);
            } else {
                strcpy(path, "/tmp/PrismaEngine");
            }
        }
        return path;
    }

    const char* Platform::GetTemporaryPath() {
        static char path[256] = {0};
        if (path[0] == '\0') {
            strcpy(path, "/tmp");
        }
        return path;
    }

    // ------------------------------------------------------------
    // 线程和同步（POSIX）
    // ------------------------------------------------------------
    #include <pthread.h>

    PlatformThreadHandle Platform::CreateThread(ThreadFunc entry, void* userData) {
        pthread_t* thread = new pthread_t;
        int result = pthread_create(thread, nullptr,
            reinterpret_cast<void*(*)(void*)>(entry), userData);
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
    // IPlatformLogger 接口实现（通用）
    // ------------------------------------------------------------
    void Platform::LogToConsole(PlatformLogLevel level, const char* tag, const char* message) {
        (void)level;
        (void)tag;
        std::cout << message << std::endl;
    }

    const char* Platform::GetLogDirectoryPath() {
        static char logPath[512] = {0};
        static bool initialized = false;

        if (!initialized) {
            const char* home = getenv("HOME");
            if (home) {
                snprintf(logPath, sizeof(logPath), "%s/.local/share/PrismaEngine/logs", home);
            } else {
                strcpy(logPath, "/tmp/PrismaEngine/logs");
            }
            initialized = true;
        }

        return logPath;
    }

#endif // !defined(_WIN32) && !defined(__ANDROID__)

// ------------------------------------------------------------
// Vulkan 支持（默认空实现）
// ------------------------------------------------------------
std::vector<const char*> Platform::GetVulkanInstanceExtensions() {
    return {};
}

bool Platform::CreateVulkanSurface(void* instance, WindowHandle windowHandle, void** outSurface) {
    (void)instance;
    (void)windowHandle;
    (void)outSurface;
    return false;
}

} // namespace Engine
