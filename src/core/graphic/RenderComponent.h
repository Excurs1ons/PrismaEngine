#pragma once
#include "Component.h"
#include "RenderCommandContext.h"
#include <DirectXMath.h>
#include <vector>
using namespace DirectX;

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
    
    // 获取顶点数据
    const float* GetVertexData() const { return m_vertices.data(); }
    uint32_t GetVertexCount() const { return m_vertexCount; }
    
    // 获取索引数据
    const uint32_t* GetIndexData() const { return m_indices.data(); }
    uint32_t GetIndexCount() const { return m_indexCount; }
    
    // 渲染方法
    virtual void Render(RenderCommandContext* context);
    
    // 设置颜色
    void SetColor(float r, float g, float b, float a = 1.0f);
    
    // 获取颜色
    XMVECTOR GetColor() const { return m_color; }
    
    // Component接口实现
    void Initialize() override;
    void Update(float deltaTime) override;
    
private:
    std::vector<float> m_vertices;
    std::vector<uint32_t> m_indices;
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    XMVECTOR m_color;
};