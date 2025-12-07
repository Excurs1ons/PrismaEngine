#pragma once
#include "Serializable.h"
#include <string>
#include <filesystem>
#include <memory>
#include "MetaData.h"
#include "SerializationVersion.h"
#include "AssetSerializer.h"

namespace Engine {
using namespace Serialization;
// Asset基类，继承自IResource和Serializable
class Asset : public Resource,public Serializable {
public:
    Asset()          = default;
    virtual ~Asset() = default;

    // Serializable接口实现
    virtual void Serialize(OutputArchive& archive) const override = 0;
    virtual void Deserialize(InputArchive& archive) override      = 0;

    // Asset特定的序列化方法
    virtual bool SerializeToFile(const std::filesystem::path& filePath,
                                 SerializationFormat format = SerializationFormat::JSON) const;

    virtual bool DeserializeFromFile(const std::filesystem::path& filePath,
                                     SerializationFormat format = SerializationFormat::JSON) {
        // 注意：这里不能直接使用AssetSerializer::DeserializeFromFile，因为我们需要修改当前对象
        // 而不是创建新对象。子类需要实现具体的逻辑。
        return false;
    }

    // 获取资产的元数据
    virtual std::string GetAssetType() const = 0;
    virtual std::string GetAssetVersion() const { return "1.0.0"; }

    // 获取和设置元数据
    const Metadata& GetMetadata() const { return m_metadata; }
    void SetMetadata(const Metadata& metadata) { m_metadata = metadata; }
    void SetMetadata(const std::string& name, const std::string& description = "") {
        m_metadata.name        = name;
        m_metadata.description = description;
    }

protected:
    Metadata m_metadata;
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

}  // namespace Engine