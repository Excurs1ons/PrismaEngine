#pragma once

#include "Logger.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <mutex>
#include "AsyncLoader.h"

namespace PrismaEngine::Core {

class AsyncResourceLoaderImplV3 : public AsyncResourceLoader {
private:
    std::unique_ptr<IImageLoader> m_imageLoader;
    std::unordered_map<std::string, IImageLoader::ImageLoadResult> m_cache;
    std::mutex m_cacheMutex;
    std::atomic<uint64_t> m_nextTaskId{1};
    std::unordered_map<std::string, std::function<void(std::shared_ptr<Graphic::ITexture>)>> m_textureCallbacks;
    std::mutex m_callbackMutex;
    
public:
    explicit AsyncResourceLoaderImplV3() {
        m_imageLoader = std::make_unique<ImageLoaderSTB>();
    }

    uint64_t loadTexture(const std::string& filePath,
                                  std::function<void(std::shared_ptr<Graphic::ITexture>)> callback) override {
        if (callback) {
            auto task = std::make_unique<TemplatedAsyncTask<ImageLoader::ImageLoadResult>>(
                filePath,
                [this, filePath, callback](const std::string& path) {
                    std::lock_guard<std::mutex> lock(m_cacheMutex);
                        
                    auto it = m_cache.find(path);
                    if (it != m_cache.end()) {
                        return it->second;
                    }
                    
                    auto result = m_imageLoader->loadFromFile(path);
                    if (result.success) {
                        m_cache[path] = result;
                        
                        {
                            std::lock_guard<std::mutex> lock(m_callbackMutex);
                            auto it = m_textureCallbacks.find(path);
                            if (it != m_textureCallbacks.end()) {
                                std::function<void(std::shared_ptr<Graphic::ITexture>)> cb = it->second;
                                if (cb) {
                                    cb(nullptr);
                                }
                            }
                        }
                    }
                    return result;
                }
            );
            
            task->setProgressCallback([this, filePath, callback](float progress) {
                if (progress >= 1.0f) {
                    std::shared_ptr<Graphic::ITexture> texture = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(m_callbackMutex);
                        m_textureCallbacks.erase(path);
                    }
                    
                    auto texture = CreateTextureFromImageData(result.width, result.height, result.channels, result.data.data());
                    callback(texture);
                }
            });
            
            return ThreadPool::GetInstance()->submit(std::move(task));
        }

        uint64_t loadModel(const std::string& filePath,
                               std::function<void(std::shared_ptr<Graphic::IMesh>)> callback) override {
            (void)filePath;
            (void)callback;
            return 0;
        }

        std::vector<uint64_t> loadTextures(const std::vector<std::string>& filePaths,
                                                   std::function<void(size_t index, std::shared_ptr<Graphic::ITexture>)> callback) override {
            std::vector<uint64_t> taskIds;
            taskIds.reserve(filePaths.size());
            
            for (size_t i = 0; i < filePaths.size(); ++i) {
                auto task = std::make_unique<TemplatedAsyncTask<ImageLoader::ImageLoadResult>>(
                    "texture_batch_" + std::to_string(i),
                    [this, filePaths, callback, i](const std::string& path) {
                        std::lock_guard<std::mutex> lock(m_callbackMutex);
                        
                        auto it = m_cache.find(path);
                        if (it != m_cache.end()) {
                            return m_imageLoader->loadFromFile(path);
                        }
                        
                        auto result = m_imageLoader->loadFromFile(path);
                        if (result.success) {
                            m_cache[path] = result;
                            
                            auto texture = CreateTextureFromImageData(result.width, result.height, result.channels, result.data.data());
                            
                            {
                                std::lock_guard<std::mutex> lock(m_callbackMutex);
                                m_textureCallbacks[path] = i;
                            }
                        }
                    }
                );
                
                taskIds.push_back(ThreadPool::GetInstance()->submit(std::move(task)));
            }
            
            return taskIds;
        }

        LoadingProgress getProgress() const override {
            LoadingProgress progress{};
            size_t total = 0, completed = 0, failed = 0;
            float currentProgress = 0.0f;
            
            for (const auto& [id, result] : m_cache) {
                if (!result.success) {
                    ++failed;
                } else {
                    ++total;
                    ++completed;
                    currentProgress += result.progress;
                }
            }
            
            progress.totalTasks = total;
            progress.completedTasks = completed;
            progress.failedTasks = failed;
            progress.progress = total > 0 ? currentProgress / total : 0.0f;
            
            return progress;
        }

        void cancelAll() override {}
        void update() override {}
        bool isLoading() const override { return false; }
    };

private:
    std::shared_ptr<Graphic::ITexture> CreateTextureFromImageData(uint32_t width, uint32_t height, 
                                                                   uint32_t channels, 
                                                                   const std::vector<uint8_t>& data) {
        
        std::shared_ptr<Graphic::ITexture> texture = nullptr;
        
        return texture;
    }
};

} // namespace Core
