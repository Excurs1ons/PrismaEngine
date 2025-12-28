#pragma once
#include "GameObject.h"
#include "Component.h"
#include "../math/MathTypes.h"
#include <vector>
#include <memory>

// 前向声明
namespace PrismaEngine::Graphic {
    class RenderCommandContext;
}

namespace Engine {
    class Material; // 前向声明
}

// 渲染组件，用于渲染几何体
class RenderComponent : public Component
{
public:
    RenderComponent();
    virtual ~RenderComponent() = default;
    
    // 设置顶点数据
    void SetVertexData(const float* vertices, uint32_t vertexCount);
    
    // 设置索引数据
    void SetIndexData(const uint32_t* indices, uint32_t indexCount);
    void SetIndexData(const uint16_t* indices, uint32_t indexCount); // 支持16位索引
    
    // 获取顶点数据
    const float* GetVertexData() const { return m_vertices.data(); }
    uint32_t GetVertexCount() const { return m_vertexCount; }
    
    // 获取索引数据
    const uint32_t* GetIndexData() const { return m_indices.data(); }
    uint32_t GetIndexCount() const { return m_indexCount; }
    
    // 渲染方法
    virtual void Render(PrismaEngine::Graphic::RenderCommandContext* context);
    
    // 设置颜色 (为了向后兼容，现在设置材质的基础颜色)
    void SetColor(float r, float g, float b, float a = 1.0f);

    // 获取颜色
    PrismaMath::vec4 GetColor() const;

    // 材质相关方法
    virtual void SetMaterial(std::shared_ptr<Engine::Material> material);
    virtual std::shared_ptr<Engine::Material> GetMaterial() const { return m_material; }
    std::shared_ptr<Engine::Material> GetOrCreateMaterial();
    
    // Component接口实现
    void Initialize() override;
    void Update(float deltaTime) override;
    
private:
    std::vector<float> m_vertices;
    std::vector<uint32_t> m_indices;
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    bool m_use16BitIndices; // 缓存索引类型，避免运行时检查
    Prisma::Color m_color;

    // 材质系统
    std::shared_ptr<Engine::Material> m_material;
};