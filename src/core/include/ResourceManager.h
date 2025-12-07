// ResourceManager.h
#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <mutex>
#include <shared_mutex>  // C++17
#include <optional>
#include <iostream>
#include <algorithm>
#include "ManagerBase.h"
#include "Resources.h"
#include "Logger.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#if defined(FindResource)
#undef FindResource
#endif
namespace Engine {
// ============================================================================
// 细粒度锁的资源管理器
// ============================================================================
class ResourceManager : public ManagerBase<ResourceManager> {
    friend class ISubSystem;
    friend class ManagerBase<ResourceManager>;
public:
    bool Initialize() override { return Initialize(std::filesystem::current_path()); }

    void Shutdown() override { UnloadAll(); }
    static constexpr const std::string GetName() { return R"(ResourceManager)"; }
    /// @brief 初始化资源管理器
    bool Initialize(const std::filesystem::path& projectRoot) {
        LOG_INFO("Resource", "资源系统正在初始化...");
        // ✅ 只锁配置相关数据
        std::unique_lock<std::mutex> configLock(m_configMutex);

        if (m_initialized) {
            LOG_INFO("Resource", "资源系统已初始化");
            return true;
        }

        if (!std::filesystem::exists(projectRoot)) {
            LOG_ERROR("Resource", "项目根目录不存在: {0}", projectRoot.string());
            return false;
        }

        m_projectRoot = std::filesystem::absolute(projectRoot);
        LOG_INFO("Resource", "资源系统根目录: {0}", m_projectRoot.string());

        configLock.unlock();  // 尽早释放锁

        // 添加搜索路径（内部会获取 m_pathsMutex）
        AddSearchPath(m_projectRoot / "Assets");
        AddSearchPath(m_projectRoot / "Assets/Shaders");
        AddSearchPath(m_projectRoot / "Assets/Textures");
        AddSearchPath(m_projectRoot / "Assets/Models");
        AddSearchPath(m_projectRoot / "Assets/Audio");


        configLock.lock();
        m_initialized = true;
        LOG_INFO("Resource", "资源系统初始化完成。");
        return true;
    }

    /// @brief 添加搜索路径
    void AddSearchPath(const std::filesystem::path& path) {
        auto absolutePath = std::filesystem::absolute(path);

        // 尝试创建目录（如果不存在）
        std::error_code ec;
        if (!std::filesystem::exists(absolutePath, ec)) {
            LOG_INFO("Resource", "搜索路径不存在，正在创建: {0}", absolutePath.string());
            std::filesystem::create_directories(absolutePath, ec);
            
            if (ec) {
                LOG_WARNING("Resource", "创建搜索路径失败: {0}, 错误码: {1}", absolutePath.string(), ec.message());
            }
        }

        // ✅ 只在修改 m_searchPaths 时加锁
        {
            std::unique_lock<std::shared_mutex> lock(m_pathsMutex);
            
            // 检查是否已经存在
            bool found = false;
            for (const auto& existingPath : m_searchPaths) {
                if (existingPath == absolutePath) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                m_searchPaths.push_back(absolutePath);
                LOG_INFO("Resource", "已添加搜索路径: {0}", absolutePath.string());
            } else {
                LOG_INFO("Resource", "搜索路径已存在: {0}", absolutePath.string());
            }
        }
    }

    /// @brief 查找资源（读操作，使用共享锁）
    std::optional<std::filesystem::path> FindResource(const std::string& relativePath) const {
        LOG_INFO("Resource", "正在查找资源: {0}", relativePath);
        std::vector<std::filesystem::path> triedPaths;
        // 1. 尝试作为绝对路径（不需要锁）
        std::filesystem::path absPath(relativePath);
        if (absPath.is_absolute()) {
            triedPaths.push_back(absPath);
            if (std::filesystem::exists(absPath)) {
                return absPath;
            } else {
                LOG_WARNING("Resource", "搜索路径不存在: {0}", absPath.string());
            }
        }
        else {
            LOG_INFO("Resource", "不是绝对路径，继续搜索");
        }
        // 2. 读取搜索路径列表（共享锁，允许多线程同时读）
        std::vector<std::filesystem::path> searchPathsCopy;
        {
            std::shared_lock<std::shared_mutex> lock(m_pathsMutex);  // ✅ 共享锁
            searchPathsCopy = m_searchPaths;                         // 快速复制
            triedPaths.push_back(absPath);
        }  // 锁立即释放
        LOG_INFO("Resource", "正在{0}个路径下搜索资源...", searchPathsCopy.size());
        // 3. 在无锁状态下搜索文件系统（耗时操作）
        for (const auto& searchPath : searchPathsCopy) {
            std::filesystem::path fullPath = searchPath / relativePath;
            fullPath                       = std::filesystem::absolute(fullPath);
            triedPaths.push_back(fullPath);
            if (std::filesystem::exists(fullPath)) {
                return std::filesystem::absolute(fullPath);
            } else {
                LOG_WARNING("Resource", "搜索路径不存在: {0}", fullPath.string());
            }
        }

        // 4. 尝试相对于项目根目录
        std::filesystem::path projectRoot;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            projectRoot = m_projectRoot;
        }

        auto projectRelative = projectRoot / relativePath;

        triedPaths.push_back(projectRelative);
        if (std::filesystem::exists(projectRelative)) {
            return std::filesystem::absolute(projectRelative);
        } 
        else
        {
            LOG_WARNING("Resource", "搜索路径不存在: {0}", projectRelative.string());
        }
        std::ostringstream oss;
        for (size_t i = 0; i < triedPaths.size(); ++i) {
            oss << triedPaths[i].string() << "\n";
        }
        LOG_ERROR("Resource", "资源未找到 \"{0}\"(以及以下路径): \n{1}", relativePath, oss.str());
        return std::nullopt;
    }

    /// @brief 加载资源
    template <typename T, typename... Args> ResourceHandle<T> Load(const std::string& relativePath, Args&&... args) {
        // 确保资源管理器已初始化
        if (!IsInitialized()) {
            // 尝试使用默认路径初始化
            std::filesystem::path defaultPath = std::filesystem::current_path();
            LOG_WARNING("Resource", "资源管理器未初始化，尝试使用默认路径初始化: {0}", defaultPath.string());
            Initialize(defaultPath);
        }
        
        // ✅ 第一阶段：检查是否已加载（共享锁）
        {
            std::shared_lock<std::shared_mutex> readLock(m_resourcesMutex);

            auto it = m_resources.find(relativePath);
            if (it != m_resources.end()) {
                auto resource = std::dynamic_pointer_cast<T>(it->second);
                if (resource && resource->IsLoaded()) {
                    LOG_INFO("Resource", "资源已加载: {0}", relativePath);
                    return ResourceHandle<T>(resource);
                }
            }
        }  // 释放读锁

        // ✅ 第二阶段：查找资源文件（不需要锁）
        auto fullPath = FindResource(relativePath);
        if (!fullPath) {
            LOG_ERROR("Resource", "找不到资源: {0}", relativePath);
            return ResourceHandle<T>();
        } else {
            LOG_INFO("Resource", "在以下位置找到资源: {0}", fullPath->string());
        }
        // ✅ 第三阶段：加载资源（耗时操作，无锁）
        auto resource = std::make_shared<T>(std::forward<Args>(args)...);
        if (!resource->Load(*fullPath)) {
            LOG_ERROR("Resource", "资源加载失败: {0}", relativePath);
            return ResourceHandle<T>();
        } else {
            LOG_INFO("Resource", "从文件加载资源: {0}", fullPath->string());
        }
        // ✅ 第四阶段：缓存资源（独占锁，仅在插入时）
        {
            std::unique_lock<std::shared_mutex> writeLock(m_resourcesMutex);

            // 双重检查：可能有其他线程已经加载了
            auto it = m_resources.find(relativePath);
            if (it != m_resources.end()) {
                auto existingResource = std::dynamic_pointer_cast<T>(it->second);
                if (existingResource && existingResource->IsLoaded()) {
                    LOG_INFO("Resource", "资源已被其他线程加载: {0}", relativePath);
                    resource->Unload();  // 丢弃我们加载的版本
                    LOG_INFO("Resource", "丢弃重复资源: {0}", relativePath);
                    return ResourceHandle<T>(existingResource);
                }
            }

            m_resources[relativePath] = resource;
            LOG_INFO("Resource", "资源已缓存: {0}", relativePath);
        }

        return ResourceHandle<T>(resource);
    }

    /// @brief 创建默认资产
    void CreateDefaultAssets();

private:
    /// @brief 创建默认网格资产
    void CreateDefaultMeshes(const std::filesystem::path& meshesDir);

    /// @brief 创建默认着色器资产
    void CreateDefaultShaders(const std::filesystem::path& shadersDir);

    /// @brief 创建默认纹理资产
    void CreateDefaultTextures(const std::filesystem::path& texturesDir);

    /// @brief 创建默认材质资产
    void CreateDefaultMaterials(const std::filesystem::path& materialsDir);

    /// @brief 卸载资源
    void Unload(const std::string& name) {
        std::unique_lock<std::shared_mutex> lock(m_resourcesMutex);

        auto it = m_resources.find(name);
        if (it != m_resources.end()) {
            it->second->Unload();
            m_resources.erase(it);
            LOG_INFO("Resource", "正在卸载资源: {0}", name);
        }
    }

    /// @brief 卸载所有资源
    void UnloadAll() {
        std::unique_lock<std::shared_mutex> lock(m_resourcesMutex);

        if (m_resources.empty())
            return;

        LOG_INFO("Resource", "正在卸载 {0} 个资源...", m_resources.size());
        for (auto& [name, resource] : m_resources) {
            resource->Unload();
        }
        m_resources.clear();

        // 同时清空搜索路径
        {
            std::unique_lock<std::shared_mutex> pathsLock(m_pathsMutex);
            m_searchPaths.clear();
        }

        std::lock_guard<std::mutex> configLock(m_configMutex);
        m_initialized = false;
    }

    /// @brief 打印统计信息
    void PrintStatistics() const {
        std::cout << "\n=== Resource Manager Statistics ===" << std::endl;

        // 读取配置
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            std::cout << "Initialized: " << (m_initialized ? "Yes" : "No") << std::endl;
            std::cout << "Project Root: " << m_projectRoot << std::endl;
        }

        // 读取搜索路径
        {
            std::shared_lock<std::shared_mutex> lock(m_pathsMutex);
            std::cout << "Search Paths: " << m_searchPaths.size() << std::endl;
            for (const auto& path : m_searchPaths) {
                std::cout << "  - " << path << std::endl;
            }
        }

        // 读取资源列表
        {
            std::shared_lock<std::shared_mutex> lock(m_resourcesMutex);
            std::cout << "Loaded Resources: " << m_resources.size() << std::endl;
            for (const auto& [name, resource] : m_resources) {
                std::cout << "  - " << name << " [" << resource->GetPath().filename().string() << "]" << std::endl;
            }
        }

        std::cout << "==================================\n" << std::endl;
    }

    bool IsInitialized() const {
        std::lock_guard<std::mutex> lock(m_configMutex);
        return m_initialized;
    }
    
    std::filesystem::path GetProjectRoot() const {
        std::lock_guard<std::mutex> lock(m_configMutex);
        return m_projectRoot;
    }
public:
    ~ResourceManager() override;

private:


    // ============================================================================
    // 成员变量 - 按访问模式分组
    // ============================================================================

    // 配置数据（很少修改，使用普通 mutex）
    mutable std::mutex m_configMutex;
    bool m_initialized = false;
    std::filesystem::path m_projectRoot;

    // 搜索路径（偶尔写，频繁读，使用读写锁）
    mutable std::shared_mutex m_pathsMutex;
    std::vector<std::filesystem::path> m_searchPaths;

    // 资源映射（频繁读写，使用读写锁）
    mutable std::shared_mutex m_resourcesMutex;
    std::unordered_map<std::string, std::shared_ptr<Resource>> m_resources;
};

}  // namespace Engine