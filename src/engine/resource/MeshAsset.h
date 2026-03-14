#pragma once

#include "../core/Asset.h"
#include "../graphic/Mesh.h"
#include <vector>

namespace Prisma {

// 网格资产类
class MeshAsset : public Asset {
public:
    MeshAsset()           = default;
    ~MeshAsset() override = default;

    // Asset接口实现
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    AssetType GetType() const override { return AssetType::Mesh; }

    // Serializable接口实现
    void Serialize(Serialization::OutputArchive& archive) const override;
    void Deserialize(Serialization::InputArchive& archive) override;

    // Asset特定方法
    std::string GetAssetType() const override { return "Mesh"; }

    // 网格属性
    const std::vector<Graphic::SubMesh>& GetSubMeshes() const { return m_subMeshes; }
    const Graphic::BoundingBox& GetBoundingBox() const { return m_boundingBox; }

    void AddSubMesh(const Graphic::SubMesh& subMesh);
    void SetBoundingBox(const Graphic::BoundingBox& boundingBox);
    void Clear();

private:
    std::vector<Graphic::SubMesh> m_subMeshes;
    Graphic::BoundingBox m_boundingBox;
};

} // namespace Prisma
