#pragma once

#include "UUID.h"
#include "Export.h"
#include "Serializable.h"
#include "MetaData.h"
#include <filesystem>
#include <string>
#include <memory>

namespace Prisma {

enum class AssetType : uint16_t {
    None = 0,
    Texture,
    Mesh,
    Model,
    Shader,
    Audio,
    Material,
    Scene,
    Script,
    Animation,
    Config,
    Tilemap,
    Unknown = 0xFFFF
};

enum class AssetState : uint8_t {
    None = 0,
    Loading,
    Loaded,
    Failed
};

class ENGINE_API Asset : public Serialization::Serializable {
public:
    UUID ID;
    std::string Name;
    std::filesystem::path Path;
    
    Asset() = default;
    virtual ~Asset() = default;

    virtual AssetType GetType() const = 0;
    virtual std::string GetAssetType() const { return "Unknown"; }
    virtual bool Load(const std::filesystem::path& path) = 0;
    virtual void Unload() { m_State = AssetState::None; }

    AssetState GetState() const { return m_State; }
    virtual bool IsLoaded() const { return m_State == AssetState::Loaded; }

    void SetName(const std::string& name) { Name = name; }

    // Serialization
    void Serialize(Serialization::OutputArchive& archive) const override {
        archive("ID", (uint64_t)ID);
        archive("Name", Name);
        archive("Path", Path.string());
        archive("Metadata", m_Metadata);
    }

    void Deserialize(Serialization::InputArchive& archive) override {
        uint64_t id;
        archive("ID", id);
        ID = UUID(id);
        archive("Name", Name);
        std::string pathStr;
        archive("Path", pathStr);
        Path = pathStr;
        archive("Metadata", m_Metadata);
    }

protected:
    AssetState m_State = AssetState::None;
    Resource::MetaData m_Metadata;

    void SetLoaded(bool loaded) { m_State = loaded ? AssetState::Loaded : AssetState::Failed; }
};

template<typename T>
class AssetHandle {
public:
    AssetHandle() = default;
    AssetHandle(std::shared_ptr<T> asset) : m_Asset(asset) {}

    T* Get() const { return m_Asset.get(); }
    T* operator->() const { return m_Asset.get(); }
    T& operator*() const { return *m_Asset; }

    bool IsValid() const { return m_Asset != nullptr && m_Asset->IsLoaded(); }
    explicit operator bool() const { return IsValid(); }

    bool operator==(const AssetHandle& other) const { return m_Asset == other.m_Asset; }
    bool operator!=(const AssetHandle& other) const { return !(*this == other); }

private:
    std::shared_ptr<T> m_Asset;
};

} // namespace Prisma
