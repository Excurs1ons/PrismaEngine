
#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "../resource/ResourceBase.h"
#include "../math/MathTypes.h"
#include "../math/Color.h"
#include <nlohmann/json.hpp>
#include "Handle.h"

// ... 其他资源类型
struct Vertex
{
    PrismaMath::vec4 position;     // 位置
    PrismaMath::vec4 normal;       // 法线
    PrismaMath::vec4 texCoord;     // 纹理坐标
    PrismaMath::vec4 tangent;      // 切线
    PrismaMath::vec4 color;         // 顶点颜色
    constexpr static uint32_t GetVertexStride() { return sizeof(Vertex); }
};

struct SubMesh {
    std::string name;
    uint32_t materialIndex; // 对应材质数组的索引
	std::vector<Vertex> vertices; // 子网格的顶点数据
	std::vector<uint32_t> indices; // 子网格的顶点索引数据
	uint32_t verticesCount() const { return static_cast<uint32_t>(vertices.size()); }
    uint32_t indicesCount() const { return static_cast<uint32_t>(indices.size()); }
    // 图形API资源句柄
    VertexBufferHandle vertexBufferHandle;
    IndexBufferHandle indexBufferHandle;
};

// 简单的包围盒结构
struct BoundingBox {
    PrismaMath::vec3 minBounds;
    PrismaMath::vec3 maxBounds;

    BoundingBox() {
        minBounds = PrismaMath::vec3(0, 0, 0);
        maxBounds = PrismaMath::vec3(0, 0, 0);
    }
    BoundingBox(const PrismaMath::vec3& minVal, const PrismaMath::vec3& maxVal) {
        minBounds = minVal;
        maxBounds = maxVal;
    }

    // 扩展包围盒以包含点
    void Encapsulate(const PrismaMath::vec3& point) {
        if (point.x < minBounds.x) minBounds.x = point.x;
        if (point.y < minBounds.y) minBounds.y = point.y;
        if (point.z < minBounds.z) minBounds.z = point.z;
        if (point.x > maxBounds.x) maxBounds.x = point.x;
        if (point.y > maxBounds.y) maxBounds.y = point.y;
        if (point.z > maxBounds.z) maxBounds.z = point.z;
    }

    // 合并另一个包围盒
    void Merge(const BoundingBox& other) {
        Encapsulate(other.minBounds);
        Encapsulate(other.maxBounds);
    }

    // 获取中心点
    [[nodiscard]] PrismaMath::vec3 GetCenter() const {
        return (minBounds + maxBounds) * 0.5f;
    }

    // 获取尺寸
    [[nodiscard]] PrismaMath::vec3 GetSize() const {
        return maxBounds - minBounds;
    }
};

class Mesh : public Engine::ResourceBase
{
    // 加载到GPU的方法
    //bool UploadToGPU(RenderDevice* device);
    //void Render(CommandBuffer* cmd, MaterialInstance* materialOverrides = nullptr);
	/// @brief 获取顶点数据的字节大小
public:
    std::vector<SubMesh> subMeshes;
    BoundingBox globalBoundingBox; // 整个Mesh的包围盒
    bool keepCpuData = false;
	static Mesh GetCubeMesh();
    static Mesh GetTriangleMesh();
	static Mesh GetQuadMesh();

    [[nodiscard]] Engine::ResourceType GetType() const override {
        return Engine::ResourceType::Mesh;
    }
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override;

private:
    bool m_isLoaded = false;
};
