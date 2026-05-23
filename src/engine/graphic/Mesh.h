#pragma once

#include "Asset.h"
#include "interfaces/RenderTypes.h"
#include "math/MathTypes.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace Prisma::Graphic {

class IBuffer;

/**
 * @brief 网格资产 (Mesh)
 *
 * 通用网格资产，支持：
 * - CPU 侧顶点数据始终保留（供路径追踪、物理等子系统使用）
 * - GPU 侧缓冲区由下游渲染系统按需创建
 * - 加载器通过扩展名自动选择（glTF/OBJ）
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

    // ── GPU 渲染数据 ──
    const std::vector<SubMeshBuffer>& GetSubMeshes() const { return m_SubMeshes; }
    const BoundingBox& GetBoundingBox() const { return m_BoundingBox; }

    void AddSubMesh(const SubMeshBuffer& subMesh) { m_SubMeshes.push_back(subMesh); }
    void SetBoundingBox(const BoundingBox& boundingBox) { m_BoundingBox = boundingBox; }

    // ── CPU 顶点数据（始终保留，供路径追踪/物理使用） ──
    struct CPUMeshData {
        std::vector<::Prisma::Vector3> positions;
        std::vector<::Prisma::Vector3> normals;   // 空 = 自动计算面法线
        std::vector<::Prisma::Vector2> uvs;        // 空 = 无 UV
        std::vector<::Prisma::Vector4> colors;     // 顶点颜色
        std::vector<uint32_t> indices;
    };

    const std::vector<CPUMeshData>& GetCPUSubMeshes() const { return m_cpuData; }
    const CPUMeshData& GetCPUMeshData(uint32_t subMeshIndex = 0) const { return m_cpuData[subMeshIndex]; }
    bool HasCPUMeshData() const { return !m_cpuData.empty(); }
    void AddCPUMeshData(const CPUMeshData& data) { m_cpuData.push_back(data); }
    void ClearCPUMeshData() { m_cpuData.clear(); }
    std::vector<CPUMeshData>& GetMutableCPUMeshData() { return m_cpuData; }

private:
    std::vector<SubMeshBuffer> m_SubMeshes;
    BoundingBox m_BoundingBox;

    // CPU 侧顶点数据（加载器填充，生命周期 = Mesh 生命周期）
    std::vector<CPUMeshData> m_cpuData;
};

} // namespace Prisma::Graphic
