
#pragma once
#define NOMINMAX
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
    PrismaMath::vec3 min;
    PrismaMath::vec3 max;

    BoundingBox() {
        min = PrismaMath::vec3(0, 0, 0);
        max = PrismaMath::vec3(0, 0, 0);
    }
    BoundingBox(const PrismaMath::vec3& minVal, const PrismaMath::vec3& maxVal) : min(minVal), maxVal) {}

    // 扩展包围盒以包含点
    void Encapsulate(const PrismaMath::vec3& point) {
        if (point.x < min.x) min.x = point.x;
        if (point.y < min.y) min.y = point.y;
        if (point.z < min.z) min.z = point.z;
        if (point.x > max.x) max.x = point.x;
        if (point.y > max.y) max.y = point.y;
        if (point.z > max.z) max.z = point.z;
    }

    // 合并另一个包围盒
    void Merge(const BoundingBox& other) {
        Encapsulate(other.min);
        Encapsulate(other.max);
    }

    // 获取中心点
    PrismaMath::vec3 GetCenter() const {
        return (min + max) * 0.5f;
    }

    // 获取尺寸
    PrismaMath::vec3 GetSize() const {
        return max - min;
    }
};

class Mesh : public ResourceBase
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

    ResourceType GetType() const override {
        return ResourceType::Mesh;
    }
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const;

private:
    bool m_isLoaded = false;
};
