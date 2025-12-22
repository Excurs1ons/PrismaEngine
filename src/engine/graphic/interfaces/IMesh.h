#pragma once

#include "RenderTypes.h"
#include "IBuffer.h"
#include <vector>
#include <memory>
#include <string>

namespace PrismaEngine::Graphic {

/// @brief 顶点结构（跨平台）
struct Vertex {
    glm::vec4 position;     // 位置 (x, y, z, w)
    glm::vec4 normal;       // 法线 (nx, ny, nz, padding)
    glm::vec4 texCoord;     // 纹理坐标 (u, v, padding1, padding2)
    glm::vec4 tangent;      // 切线 (tx, ty, tz, handedness)
    glm::vec4 color;        // 顶点颜色 (r, g, b, a)

    static constexpr size_t GetVertexStride() { return sizeof(Vertex); }
};

/// @brief 子网格
struct SubMesh {
    std::string name;
    uint32_t materialIndex;  // 对应材质的索引
    uint32_t baseVertex;     // 起始顶点索引
    uint32_t baseIndex;      // 起始索引位置
    uint32_t indexCount;     // 索引数量
    uint32_t vertexCount;    // 顶点数量

    // 平台特定的缓冲区（通过接口访问）
    std::shared_ptr<IBuffer> vertexBuffer;
    std::shared_ptr<IBuffer> indexBuffer;
    bool use16BitIndices;    // 是否使用16位索引
};

/// @brief 包围盒
struct BoundingBox {
    glm::vec3 minBounds;
    glm::vec3 maxBounds;

    BoundingBox() : minBounds(0, 0, 0), maxBounds(0, 0, 0) {}
    BoundingBox(const glm::vec3& min, const glm::vec3& max) : minBounds(min), maxBounds(max) {}

    /// @brief 扩展包围盒以包含点
    void Encapsulate(const glm::vec3& point) {
        minBounds = glm::min(minBounds, point);
        maxBounds = glm::max(maxBounds, point);
    }

    /// @brief 合并另一个包围盒
    void Merge(const BoundingBox& other) {
        minBounds = glm::min(minBounds, other.minBounds);
        maxBounds = glm::max(maxBounds, other.maxBounds);
    }

    /// @brief 获取中心点
    glm::vec3 GetCenter() const {
        return (minBounds + maxBounds) * 0.5f;
    }

    /// @brief 获取尺寸
    glm::vec3 GetSize() const {
        return maxBounds - minBounds;
    }

    /// @brief 获取半径
    float GetRadius() const {
        glm::vec3 center = GetCenter();
        glm::vec3 extents = maxBounds - center;
        return glm::length(extents);
    }
};

/// @brief 网格抽象接口
class IMesh {
public:
    virtual ~IMesh() = default;

    /// @brief 获取子网格数量
    /// @return 子网格数量
    virtual uint32_t GetSubMeshCount() const = 0;

    /// @brief 获取子网格
    /// @param index 子网格索引
    /// @return 子网格数据
    virtual const SubMesh* GetSubMesh(uint32_t index) const = 0;

    /// @brief 添加子网格
    /// @param subMesh 子网格数据
    /// @return 子网格索引
    virtual uint32_t AddSubMesh(const SubMesh& subMesh) = 0;

    /// @brief 获取全局包围盒
    /// @return 包围盒
    virtual const BoundingBox& GetBoundingBox() const = 0;

    /// @brief 更新包围盒
    virtual void UpdateBoundingBox() = 0;

    /// @brief 绑定网格到渲染管线
    /// @param commandBuffer 命令缓冲区
    /// @param subMeshIndex 子网格索引
    virtual void Bind(class ICommandBuffer* commandBuffer, uint32_t subMeshIndex = 0) = 0;

    /// @brief 绘制网格
    /// @param commandBuffer 命令缓冲区
    /// @param subMeshIndex 子网格索引
    virtual void Draw(class ICommandBuffer* commandBuffer, uint32_t subMeshIndex = 0) = 0;

    /// @brief 绘制实例化网格
    /// @param commandBuffer 命令缓冲区
    /// @param instanceCount 实例数量
    /// @param subMeshIndex 子网格索引
    virtual void DrawInstanced(class ICommandBuffer* commandBuffer, uint32_t instanceCount, uint32_t subMeshIndex = 0) = 0;

    /// @brief 获取网格名称
    /// @return 网格名称
    virtual const std::string& GetName() const = 0;

    /// @brief 设置网格名称
    /// @param name 网格名称
    virtual void SetName(const std::string& name) = 0;

    /// @brief 是否保留CPU数据
    /// @param keep 是否保留
    virtual void SetKeepCpuData(bool keep) = 0;

    /// @brief 是否已经上传到GPU
    /// @return 是否在GPU上
    virtual bool IsUploaded() const = 0;

    /// @brief 上传到GPU
    /// @param device 渲染设备
    /// @return 是否成功
    virtual bool UploadToGPU(class IRenderDevice* device) = 0;

    /// @brief 从GPU卸载
    virtual void UnloadFromGPU() = 0;
};

} // namespace PrismaEngine::Graphic