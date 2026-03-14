#pragma once

#include "Asset.h"
#include "interfaces/RenderTypes.h"
#include <vector>
#include <memory>

namespace Prisma::Graphic {

class IBuffer;

/**
 * @brief 网格资产 (Mesh)
 */
class ENGINE_API Mesh : public Prisma::Asset {
public:
    Mesh() = default;
    ~Mesh() override = default;

    // Asset 接口
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override { return !m_SubMeshes.empty(); }
    Prisma::AssetType GetType() const override { return Prisma::AssetType::Mesh; }

    // 访问器
    const std::vector<SubMeshBuffer>& GetSubMeshes() const { return m_SubMeshes; }
    const BoundingBox& GetBoundingBox() const { return m_BoundingBox; }

private:
    std::vector<SubMeshBuffer> m_SubMeshes;
    BoundingBox m_BoundingBox;
};

} // namespace Prisma::Graphic
