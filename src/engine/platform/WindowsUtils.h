#pragma once
#include <string>
#include <filesystem>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

enum class FileAction {
    Unknown,
    Added,
    Removed,
    Modified,
    RenamedOldName,
    RenamedNewName
};

using FileChangeCallback = std::function<void(const std::wstring&, FileAction)>;

class DirectoryWatcher {
public:
    DirectoryWatcher();
    ~DirectoryWatcher();

    // 禁止拷贝
    DirectoryWatcher(const DirectoryWatcher&) = delete;
    DirectoryWatcher& operator=(const DirectoryWatcher&) = delete;

    /**
     * @brief 开始监控指定目录
     * 
     * @param directory 要监控的目录路径
     * @param callback 当文件发生变化时调用的回调函数。注意：回调函数在后台线程中执行，请确保线程安全。
     * @return true 如果启动成功
     * @return false 如果启动失败（例如目录不存在或已经开始监控）
     */
    bool Start(const std::wstring& directory, FileChangeCallback callback);
    bool Start(const std::filesystem::path& directory, FileChangeCallback callback);

    /**
     * @brief 停止监控
     * 
     * 阻塞直到监控线程退出。析构函数会自动调用此方法。
     */
    void Stop();

private:
    void WatchLoop();

    std::wstring m_directory;
    FileChangeCallback m_callback;
    std::thread m_watchThread;
    std::atomic<bool> m_running;
    void* m_hDir; // HANDLE
    void* m_stopEvent; // HANDLE 用于通知线程停止
};
