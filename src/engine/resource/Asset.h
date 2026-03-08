#pragma once
#include "../core/AssetBase.h"
#include "AssetSerializer.h"
#include "MetaData.h"
#include "Serializable.h"
#include "SerializationVersion.h"
#include <filesystem>
#include <memory>
#include <string>

namespace PrismaEngine {

using namespace Serialization;

/// <summary>
/// Asset基类，继承自AssetBase和Serializable
/// </summary>
class ENGINE_API Asset : public AssetBase, public Serializable {
public:
    Asset()          = default;
    virtual ~Asset() = default;

    // Serializable接口实现
    void Serialize(OutputArchive& archive) const override = 0;
    void Deserialize(InputArchive& archive) override      = 0;

    // Asset特定的序列化方法
    virtual bool SerializeToFile(const std::filesystem::path& filePath,
                                 SerializationFormat format = SerializationFormat::JSON) const;

    virtual bool DeserializeFromFile(const std::filesystem::path& filePath,
                                     SerializationFormat format = SerializationFormat::JSON);

    // 获取资产的元数据
    virtual std::string GetAssetType() const = 0;
    virtual std::string GetAssetVersion() const { return "1.0.0"; }

    // 获取和设置元数据
    const Resource::MetaData& GetMetadata() const { return m_metadata; }
    void SetMetadata(const Resource::MetaData& metadata) { m_metadata = metadata; }
    void SetMetadata(const std::string& name, const std::string& description = "") {
        m_metadata.name        = name;
        m_metadata.description = description;
    }

protected:
    Resource::MetaData m_metadata;
};

// 资产工厂接口
template <typename T> class AssetFactory {
public:
    virtual ~AssetFactory()                                                                             = default;
    virtual std::unique_ptr<T> CreateAsset()                                                            = 0;
    virtual std::unique_ptr<T> LoadAsset(const std::filesystem::path& path)                             = 0;
    virtual std::unique_ptr<T> DeserializeAsset(const std::filesystem::path& path,
                                                SerializationFormat format = SerializationFormat::JSON) = 0;
};

// 默认资产工厂实现
template <typename T> class DefaultAssetFactory : public AssetFactory<T> {
public:
    std::unique_ptr<T> CreateAsset() override { return std::make_unique<T>(); }

    std::unique_ptr<T> LoadAsset(const std::filesystem::path& path) override {
        auto asset = std::make_unique<T>();
        if (asset->Load(path)) {
            return asset;
        }
        return nullptr;
    }

    std::unique_ptr<T> DeserializeAsset(const std::filesystem::path& path,
                                        SerializationFormat format = SerializationFormat::JSON) override {
        return AssetSerializer::template DeserializeFromFile<T>(path, format);
    }
};

}  // namespace PrismaEngine
