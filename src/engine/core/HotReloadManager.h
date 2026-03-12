#pragma once

#include "Logger.h"
#include <filesystem>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>

namespace PrismaEngine::Core {

class HotReloadManager {
public:
    struct AssetChange {
        std::string path;
        std::filesystem::file_time_type lastModified;
    std::function<void()> reloadCallback;
    };

    HotReloadManager() = default;
    ~HotReloadManager() = default;

    void Initialize() {
        m_running.store(true);
        m_watcherThread = std::thread(&HotReloadManager::WatchFilesystem, this);
        m_watcherThread.detach();
        LOG_INFO("HotReload", "Hot reload system initialized");
    }

    void Shutdown() {
        m_running.store(false);
        if (m_watcherThread.joinable()) {
            m_watcherThread.join();
        }
    }

    void WatchFilesystem() {
        std::unordered_map<std::string, AssetChange> watchedFiles;
        const auto watchInterval = std::chrono::seconds(1);
        
        while (m_running.load()) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator("assets")) {
                std::string path = entry.path().string();
                
                if (!std::filesystem::is_directory(path)) {
                    continue;
                }
                
                AssetChange change;
                change.path = path;
                change.lastModified = std::filesystem::last_write_time(path);
                
                watchedFiles[path] = change;
                change.reloadCallback = nullptr;
            }
            
            std::this_thread::sleep_for(watchInterval);
        }
    }

    void RegisterAsset(const std::string& path, std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        AssetChange change;
        change.path = path;
        change.lastModified = std::filesystem::last_write_time(path);
        change.reloadCallback = callback;
        
        watchedFiles[path] = change;
        
        LOG_INFO("HotReload", "Registered asset for hot reload: {}", path);
    }

    void UnregisterAsset(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        watchedFiles.erase(path);
        LOG_INFO("HotReload", "Unregistered asset: {}", path);
    }

    void TriggerReload(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = watchedFiles.find(path);
        if (it != watchedFiles.end() {
            it->second.reloadCallback();
        }
    }

private:
    std::unordered_map<std::string, AssetChange> watchedFiles;
    std::mutex m_mutex;
    std::thread m_watcherThread;
    std::atomic<bool> m_running;
};

} // namespace Core
