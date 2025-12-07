
#pragma once
#define NOMINMAX
#include "Resources.h"
#include "RenderBackendDirectX12.h"
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
#include <nlohmann/json.hpp>
using namespace Engine;
// 基础句柄类型
class Handle {
private:
    std::optional<uint32_t> m_id;

public:
    Handle() = default;
    explicit Handle(uint32_t id) : m_id(id) {}

    bool IsValid() const { return m_id.has_value(); }
    explicit operator bool() const { return IsValid(); }

    uint32_t GetId() const { return m_id.value_or(0xFFFFFFFF); }

    static Handle Invalid() { return Handle(); }
};

// 类型安全的派生句柄
struct VertexBufferHandle : public Handle {};
struct IndexBufferHandle : public Handle {};
struct TextureHandle : public Handle {};

// ... 其他资源类型
struct Vertex
{
    XMFLOAT4 position;     // 位置
    XMFLOAT4 normal;       // 法线
    XMFLOAT4 texCoord;     // 纹理坐标
    XMFLOAT4 tangent;      // 切线
    XMVECTORF32 color;        // 顶点颜色
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

class Mesh : public Resource
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
