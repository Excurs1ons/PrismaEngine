#pragma once

#include "Asset.h"
#include "Mesh.h"
#include <vector>

namespace PrismaEngine {

// 网格资产类
class MeshAsset : public Asset {
public:
    MeshAsset()           = default;
    ~MeshAsset() override = default;

    // IResource接口实现
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override { return m_isLoaded; }
    AssetType GetType() const override { return AssetType::Mesh; }

    // Serializable接口实现
    void Serialize(Serialization::OutputArchive& archive) const override;
    void Deserialize(Serialization::InputArchive& archive) override;

    // 重写Asset的DeserializeFromFile方法
    bool
    DeserializeFromFile(const std::filesystem::path& path,
                        Serialization::SerializationFormat format = Serialization::SerializationFormat::JSON) override;

    // Asset特定方法
    std::string GetAssetType() const override { return "Mesh"; }
    std::string GetAssetVersion() const override { return "1.0.0"; }

    // 网格属性
    const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
    const BoundingBox& GetBoundingBox() const { return m_boundingBox; }

    void AddSubMesh(const SubMesh& subMesh);
    void SetBoundingBox(const BoundingBox& boundingBox);
    void Clear();

private:
    std::vector<SubMesh> m_subMeshes;
    BoundingBox m_boundingBox;
    bool m_isLoaded = false;
};

}  // namespace Engine