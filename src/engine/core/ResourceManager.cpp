#include "ResourceManager.h"
#include "Logger.h"
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <thread>

namespace Engine {
namespace Core {

ResourceManager::ResourceManager()
{
    LOG_INFO("ResourceManager", "资源管理器初始化");

    // 添加默认搜索路径
    AddSearchPath("assets/");
    AddSearchPath("resources/");
    AddSearchPath("textures/");
    AddSearchPath("models/");
    AddSearchPath("shaders/");
    AddSearchPath("audio/");
}

ResourceManager::~ResourceManager()
{
    LOG_INFO("ResourceManager", "资源管理器关闭中...");

    // 停止后台任务
    m_running = false;

    // 等待所有异步任务完成
    for (auto& task : m_asyncTasks) {
        if (task.valid()) {
            task.wait();
        }
    }

    // 清理所有资源
    ClearCache();

    LOG_INFO("ResourceManager", "资源管理器已关闭");
}

void ResourceManager::RegisterLoader(ResourceType type, std::unique_ptr<IResourceLoader> loader)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (loader) {
        m_loaders[type] = std::move(loader);
        LOG_INFO("ResourceManager", "注册资源加载器，类型: {0}", static_cast<uint32_t>(type));
    }
}

std::shared_ptr<IResource> ResourceManager::LoadResource(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查缓存
    auto it = m_resources.find(path);
    if (it != m_resources.end()) {
        // 更新最后使用时间
        it->second->GetLastUsedTime();
        return it->second;
    }

    // 查找完整路径
    std::string fullPath = FindResourcePath(path);
    if (fullPath.empty()) {
        LOG_ERROR("ResourceManager", "找不到资源: {0}", path);
        return nullptr;
    }

    // 根据扩展名确定资源类型
    std::string ext = std::filesystem::path(fullPath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    ResourceType type = ResourceType::Unknown;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".dds") {
        type = ResourceType::Texture;
    } else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
        type = ResourceType::Mesh;
    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        type = ResourceType::Audio;
    } else if (ext == ".hlsl" || ext == ".vert" || ext == ".frag") {
        type = ResourceType::Shader;
    } else if (ext == ".mat") {
        type = ResourceType::Material;
    } else if (ext == ".anim") {
        type = ResourceType::Animation;
    } else if (ext == ".scene") {
        type = ResourceType::Scene;
    } else if (ext == ".lua") {
        type = ResourceType::Script;
    } else if (ext == ".json" || ext == ".xml" || ext == ".ini") {
        type = ResourceType::Config;
    }

    if (type == ResourceType::Unknown) {
        LOG_ERROR("ResourceManager", "未知的资源类型: {0}", ext);
        return nullptr;
    }

    // 获取对应的加载器
    auto loaderIt = m_loaders.find(type);
    if (loaderIt == m_loaders.end()) {
        LOG_ERROR("ResourceManager", "没有注册的资源加载器，类型: {0}", static_cast<uint32_t>(type));
        return nullptr;
    }

    // 创建资源
    auto resource = loaderIt->second->CreateResource(fullPath);
    if (!resource) {
        LOG_ERROR("ResourceManager", "创建资源失败: {0}", fullPath);
        return nullptr;
    }

    // 加载资源
    resource->SetState(ResourceState::Loading);
    if (!resource->Load()) {
        resource->SetState(ResourceState::Failed);
        LOG_ERROR("ResourceManager", "加载资源失败: {0}", fullPath);
        return nullptr;
    }

    resource->SetState(ResourceState::Loaded);

    // 缓存资源
    if (m_cacheEnabled) {
        m_resources[path] = resource;

        // 检查内存限制
        UpdateStats();
        if (m_stats.totalMemoryUsage > m_cacheSizeLimit) {
            EvictLRU();
        }
    }

    LOG_DEBUG("ResourceManager", "成功加载资源: {0}", path);
    return resource;
}

std::future<std::shared_ptr<IResource>> ResourceManager::LoadResourceAsync(const std::string& path)
{
    return std::async(std::launch::async, [this, path]() {
        return LoadResource(path);
    });
}

std::shared_ptr<IResource> ResourceManager::GetResource(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_resources.find(path);
    if (it != m_resources.end()) {
        return it->second;
    }

    return nullptr;
}

void ResourceManager::ReleaseResource(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_resources.find(path);
    if (it != m_resources.end()) {
        if (it->second->GetRefCount() == 1) {
            LOG_DEBUG("ResourceManager", "释放资源: {0}", path);
            m_resources.erase(it);
            UpdateStats();
        }
    }
}

void ResourceManager::Preload(const std::string& pattern)
{
    LOG_INFO("ResourceManager", "预加载资源: {0}", pattern);

    // 遍历所有搜索路径
    for (const auto& searchPath : m_searchPaths) {
        std::filesystem::path basePath(searchPath);
        if (!std::filesystem::exists(basePath)) {
            continue;
        }

        // 递归查找匹配的文件
        for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();

                // 简单的模式匹配
                if (filePath.find(pattern) != std::string::npos) {
                    LoadResourceAsync(filePath);
                }
            }
        }
    }
}

void ResourceManager::UnloadUnused()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t unloaded = 0;
    auto it = m_resources.begin();
    while (it != m_resources.end()) {
        if (it->second->GetRefCount() == 1) {
            LOG_DEBUG("ResourceManager", "卸载未使用资源: {0}", it->first);
            it->second->Unload();
            it = m_resources.erase(it);
            unloaded++;
        } else {
            ++it;
        }
    }

    UpdateStats();
    LOG_INFO("ResourceManager", "卸载了 {0} 个未使用的资源", unloaded);
}

void ResourceManager::AddSearchPath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_searchPaths.push_back(path);
    LOG_DEBUG("ResourceManager", "添加搜索路径: {0}", path);
}

void ResourceManager::ClearSearchPaths()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_searchPaths.clear();
    LOG_DEBUG("ResourceManager", "清空所有搜索路径");
}

ResourceManager::ResourceStats ResourceManager::GetStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UpdateStats();
    return m_stats;
}

void ResourceManager::SetMemoryLimit(uint64_t limit)
{
    m_memoryLimit = limit;
    LOG_INFO("ResourceManager", "设置内存限制: {0} MB", limit / (1024 * 1024));
}

void ResourceManager::SetCacheEnabled(bool enabled)
{
    m_cacheEnabled = enabled;
    LOG_INFO("ResourceManager", "缓存: {0}", enabled ? "启用" : "禁用");
}

void ResourceManager::SetCacheSizeLimit(uint64_t size)
{
    m_cacheSizeLimit = size;
    LOG_INFO("ResourceManager", "设置缓存大小限制: {0} MB", size / (1024 * 1024));
}

void ResourceManager::ClearCache()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 卸载所有资源
    for (auto& pair : m_resources) {
        pair.second->Unload();
    }

    m_resources.clear();
    UpdateStats();

    LOG_INFO("ResourceManager", "缓存已清空");
}

bool ResourceManager::SaveResource(const std::string& path, std::shared_ptr<IResource> resource)
{
    // TODO: 实现资源保存功能
    LOG_WARNING("ResourceManager", "资源保存功能尚未实现: {0}", path);
    return false;
}

void ResourceManager::EnableHotReload(bool enable)
{
    m_hotReloadEnabled = enable;
    LOG_INFO("ResourceManager", "热重载: {0}", enable ? "启用" : "禁用");
}

void ResourceManager::UpdateStats() const
{
    m_stats.totalResources = static_cast<uint32_t>(m_resources.size());
    m_stats.loadedResources = 0;
    m_stats.loadingResources = 0;
    m_stats.failedResources = 0;
    m_stats.totalMemoryUsage = 0;
    m_stats.textureMemoryUsage = 0;
    m_stats.meshMemoryUsage = 0;
    m_stats.audioMemoryUsage = 0;

    for (const auto& pair : m_resources) {
        const auto& resource = pair.second;

        switch (resource->GetState()) {
        case ResourceState::Loaded:
            m_stats.loadedResources++;
            break;
        case ResourceState::Loading:
            m_stats.loadingResources++;
            break;
        case ResourceState::Failed:
            m_stats.failedResources++;
            break;
        default:
            break;
        }

        uint64_t size = resource->GetSize();
        m_stats.totalMemoryUsage += size;

        switch (resource->GetType()) {
        case ResourceType::Texture:
            m_stats.textureMemoryUsage += size;
            break;
        case ResourceType::Mesh:
            m_stats.meshMemoryUsage += size;
            break;
        case ResourceType::Audio:
            m_stats.audioMemoryUsage += size;
            break;
        default:
            break;
        }
    }
}

std::string ResourceManager::FindResourcePath(const std::string& relativePath) const
{
    // 如果是绝对路径，直接返回
    if (std::filesystem::path(relativePath).is_absolute()) {
        return std::filesystem::exists(relativePath) ? relativePath : "";
    }

    // 在搜索路径中查找
    for (const auto& searchPath : m_searchPaths) {
        std::string fullPath = searchPath + relativePath;
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }

    return "";
}

void ResourceManager::EvictLRU()
{
    // 收集所有资源并按最后使用时间排序
    std::vector<std::pair<std::string, std::shared_ptr<IResource>>> resources;
    for (const auto& pair : m_resources) {
        // 跳过引用计数大于1的资源（仍在使用）
        if (pair.second->GetRefCount() == 1) {
            resources.emplace_back(pair.first, pair.second);
        }
    }

    // 按最后使用时间排序
    std::sort(resources.begin(), resources.end(),
              [](const auto& a, const auto& b) {
                  return a.second->GetLastUsedTime() < b.second->GetLastUsedTime();
              });

    // 移除最旧的资源，直到内存使用低于限制
    uint64_t targetSize = m_cacheSizeLimit * 0.8; // 保留20%空间
    for (const auto& pair : resources) {
        if (m_stats.totalMemoryUsage <= targetSize) {
            break;
        }

        m_resources.erase(pair.first);
        LOG_DEBUG("ResourceManager", "LRU移除资源: {0}", pair.first);
    }

    UpdateStats();
}

} // namespace Core
} // namespace Engine