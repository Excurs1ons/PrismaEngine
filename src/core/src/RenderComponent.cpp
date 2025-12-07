#include "RenderComponent.h"
#include "Transform.h"
#include "Logger.h"

RenderComponent::RenderComponent()
    : m_vertexCount(0)
    , m_indexCount(0)
    , m_color(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f)) // 默认白色
{
}

void RenderComponent::SetVertexData(const float* vertices, uint32_t vertexCount)
{
    m_vertices.clear();
    m_vertices.resize(vertexCount * 7); // 假设每个顶点有7个float: 位置(x,y,z) + 颜色(r,g,b,a)
    memcpy(m_vertices.data(), vertices, vertexCount * 7 * sizeof(float));
    m_vertexCount = vertexCount;
}

void RenderComponent::SetIndexData(const uint32_t* indices, uint32_t indexCount)
{
    m_indices.clear();
    m_indices.resize(indexCount);
    memcpy(m_indices.data(), indices, indexCount * sizeof(uint32_t));
    m_indexCount = indexCount;
}

void RenderComponent::Render(RenderCommandContext* context)
{
    if (!context || m_vertexCount == 0)
        return;
        
    // 设置变换矩阵
    if (auto* transform = m_owner->transform()) {
        float* matrix = transform->GetMatrix();
        XMMATRIX worldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(matrix));
        context->SetConstantBuffer("World", worldMatrix);
    }
    
    // 设置颜色
    context->SetConstantBuffer("ObjectColor", reinterpret_cast<const float*>(&m_color), 4);

    // 绑定顶点缓冲区到渲染后端（将数据复制到后端的 per-frame upload buffer）
    // 顶点布局: 7 floats per vertex (x,y,z,r,g,b,a)
    const uint32_t stride = 7 * sizeof(float);
    const uint32_t sizeInBytes = m_vertexCount * stride;
    context->SetVertexBuffer(m_vertices.data(), sizeInBytes, stride);
    
    // 绘制
    if (m_indexCount > 0) {
        context->DrawIndexed(m_indexCount);
    } else {
        context->Draw(m_vertexCount);
    }
}

void RenderComponent::SetColor(float r, float g, float b, float a)
{
    m_color = XMVectorSet(r, g, b, a);
}

void RenderComponent::Initialize()
{
    LOG_DEBUG("RenderComponent", "RenderComponent initialized for GameObject: {0}", m_owner ? m_owner->name : "Unknown");
}

void RenderComponent::Update(float deltaTime)
{
    // 渲染组件通常不需要每帧更新，除非有动画等
}