#pragma once

#include "UUID.h"
#include "Export.h"
#include "Serializable.h"
#include "MetaData.h"
#include <filesystem>
#include <string>
#include <memory>

namespace Prisma {

enum class AssetType {
    None = 0,
    Texture,
    Mesh,
    Material,
    Shader,
    Scene,
    Audio,
    Tilemap
};

/**
 * @brief 资产基类
 */
class ENGINE_API Asset : public Serialization::ISerializable {
public:
    Asset() = default;
    virtual ~Asset() = default;

    virtual AssetType GetType() const = 0;
    virtual std::string GetAssetType() const { return "Unknown"; }
    void SetName(const std::string& name) { m_Name = name; }
    const std::string& GetName() const { return m_Name; }
    virtual bool Load(const std::filesystem::path& path) = 0;
    virtual void Unload() = 0;

    virtual bool IsLoaded() const { return m_IsLoaded; }
    void SetLoaded(bool loaded) { m_IsLoaded = loaded; }

    bool IsDirty() const { return m_IsDirty; }
    void SetDirty(bool dirty) { m_IsDirty = dirty; }

    UUID GetHandle() const { return m_Handle; }
    void SetHandle(UUID handle) { m_Handle = handle; }

    const std::filesystem::path& GetPath() const { return m_Path; }
    void SetPath(const std::filesystem::path& path) { m_Path = path; }

    // ISerializable 接口实现
    void Serialize(Serialization::OutputArchive& archive) const override {
        archive("Handle", (uint64_t)m_Handle);
        archive("Name", m_Name);
    }
    void Deserialize(Serialization::InputArchive& archive) override {
        uint64_t handle = 0;
        archive("Handle", handle);
        m_Handle = handle;
        archive("Name", m_Name);
    }

protected:
    UUID m_Handle;
    std::filesystem::path m_Path;
    std::string m_Name;
    bool m_IsLoaded = false;
    bool m_IsDirty  = false;
};

/**
 * @brief 资产句柄 (强引用)
 */
template <typename T>
class AssetHandle {
public:
    AssetHandle() = default;
    AssetHandle(std::shared_ptr<T> asset) : m_Asset(asset) {}

    bool IsValid() const { return m_Asset != nullptr; }
    T* operator->() { return m_Asset.get(); }
    const T* operator->() const { return m_Asset.get(); }
    T& operator*() { return *m_Asset; }
    const T& operator*() const { return *m_Asset; }

    std::shared_ptr<T> Get() const { return m_Asset; }

    bool operator==(const AssetHandle& other) const { return m_Asset == other.m_Asset; }
    bool operator!=(const AssetHandle& other) const { return !(*this == other); }

private:
    std::shared_ptr<T> m_Asset;
};

} // namespace Prisma
