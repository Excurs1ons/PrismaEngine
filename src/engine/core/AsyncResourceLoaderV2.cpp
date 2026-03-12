#include "ImageLoaderSTB.h"
#include "Logger.h"
#include <functional>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace PrismaEngine::Core {

class AsyncResourceLoaderImplV2 : public AsyncResourceLoader {
private:
    std::unique_ptr<IImageLoader> m_imageLoader;
    std::unordered_map<std::string, IImageLoader::ImageLoadResult> m_cache;
    std::mutex m_cacheMutex;
    std::atomic<uint64_t> m_nextTaskId{1};
    
public:
    explicit AsyncResourceLoaderImplV2() {
        m_imageLoader = std::make_unique<ImageLoaderSTB>();
    }
    
    uint64_t loadTexture(const std::string& filePath,
                                  std::function<void(std::shared_ptr<Graphic::ITexture>)> callback) override {
        if (callback) {
            auto task = std::make_unique<TemplatedAsyncTask<ImageLoader::ImageLoadResult>>(
                filePath,
                [this, filePath](const std::string& path) {
                    std::lock_guard<std::mutex> lock(m_cacheMutex);
                    
                    auto it = m_cache.find(path);
                    if (it != m_cache.end()) {
                        return it->second;
                    }
                    
                    auto result = m_imageLoader->loadFromFile(path);
                    if (result.success) {
                        m_cache[path] = result;
                    }
                    return result;
                }
            );
            
            task->setProgressCallback([this, callback](float progress) {
                if (progress >= 1.0f) {
                    callback(nullptr);
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
                    [this, filePaths, callback](const std::string& path) {
                        std::lock_guard<std::mutex> lock(m_cacheMutex);
                        
                        auto it = m_cache.find(path);
                        if (it != m_cache.end()) {
                            return m_imageLoader->loadFromFile(path);
                        }
                        
                        return it->second;
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

} // namespace PrismaEngine::Core
