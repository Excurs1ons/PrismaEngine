#pragma once
#include <filesystem>
#include <iostream>

namespace Engine {
// ============================================================================
// 资源类型枚举
// ============================================================================
enum class ResourceType { Unknown, Shader, Texture, Mesh, Model, Audio, Material, Config };

// 前向声明
class ResourceManager;

// 资源基类（同前）
class ResourceBase {
public:
    std::string name;
    virtual ~ResourceBase()                                 = default;
    virtual bool Load(const std::filesystem::path& path) = 0;
    virtual void Unload()                                = 0;
    virtual bool IsLoaded() const                        = 0;
    virtual ResourceType GetType() const                 = 0;

    const std::filesystem::path& GetPath() const { return m_path; }
    const std::string& GetName() const { return m_name; }

protected:
    std::filesystem::path m_path;
    std::string m_name;
    bool m_isLoaded = false;

    // 允许 ResourceManager 访问 protected 成员
    friend class ResourceManager;
};

// 资源句柄（同前）
template <typename T> class ResourceHandle {
public:
    ResourceHandle() = default;
    explicit ResourceHandle(std::shared_ptr<T> resource) : m_resource(resource) {}

    T* Get() const { return m_resource.get(); }
    T* operator->() const { return Get(); }
    T& operator*() const { return *Get(); }
    bool IsValid() const { return m_resource != nullptr && m_resource->IsLoaded(); }
    explicit operator bool() const { return IsValid(); }

private:
    std::shared_ptr<T> m_resource;
};

}  // namespace Engine