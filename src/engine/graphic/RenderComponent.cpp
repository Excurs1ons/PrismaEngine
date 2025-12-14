#include "RenderComponent.h"
#include "Material.h"
#include "Transform.h"
#include "Logger.h"

RenderComponent::RenderComponent()
    : m_vertexCount(0)
    , m_indexCount(0)
    , m_use16BitIndices(true) // 默认使用16位索引
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

    // 检查是否可以使用16位索引
    m_use16BitIndices = true;
    for (uint32_t i = 0; i < indexCount; ++i) {
        if (indices[i] > 65535) {
            m_use16BitIndices = false;
            break;
        }
    }
}

void RenderComponent::SetIndexData(const uint16_t* indices, uint32_t indexCount)
{
    m_indices.clear();
    m_indices.resize(indexCount);
    // 将16位索引转换为32位存储
    for (uint32_t i = 0; i < indexCount; ++i) {
        m_indices[i] = static_cast<uint32_t>(indices[i]);
    }
    m_indexCount = indexCount;
    m_use16BitIndices = true; // 16位输入，使用16位索引
}

void RenderComponent::Render(RenderCommandContext* context)
{
    if (!context || m_vertexCount == 0)
        return;

    // 应用材质 (这会设置颜色、纹理等参数)
    auto material = GetOrCreateMaterial();
    if (material) {
        material->Apply(context);
    }

    // 设置世界矩阵 (寄存器 b1)
    if (auto* transform = m_owner->transform()) {
        float* matrix = transform->GetMatrix();
        XMMATRIX worldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(matrix));
        // 转置矩阵以适应HLSL的列主序要求
        worldMatrix = XMMatrixTranspose(worldMatrix);
        context->SetConstantBuffer("World", reinterpret_cast<const float*>(&worldMatrix), 16);
    }

    // 绑定顶点缓冲区到渲染后端（将数据复制到后端的 per-frame upload buffer）
    // 顶点布局: 7 floats per vertex (x,y,z,r,g,b,a)
    const uint32_t stride = 7 * sizeof(float);
    const uint32_t vertexSizeInBytes = m_vertexCount * stride;
    context->SetVertexBuffer(m_vertices.data(), vertexSizeInBytes, stride);

    // 如果有索引数据，绑定索引缓冲区
    if (m_indexCount > 0) {
        if (m_use16BitIndices) {
            // 转换为16位索引数组
            std::vector<uint16_t> indices16(m_indexCount);
            for (uint32_t i = 0; i < m_indexCount; ++i) {
                indices16[i] = static_cast<uint16_t>(m_indices[i]);
            }
            context->SetIndexBuffer(indices16.data(), m_indexCount * sizeof(uint16_t), true);
        } else {
            context->SetIndexBuffer(m_indices.data(), m_indexCount * sizeof(uint32_t), false);
        }

        // 执行索引绘制
        context->DrawIndexed(m_indexCount);
    } else {
        // 执行普通顶点绘制
        context->Draw(m_vertexCount);
    }
}

void RenderComponent::SetColor(float r, float g, float b, float a)
{
    m_color = XMVectorSet(r, g, b, a);

    // 如果已有材质，更新其基础颜色
    if (m_material) {
        m_material->SetBaseColor(r, g, b, a);
    }
}

// 获取颜色
XMVECTOR RenderComponent::GetColor() const
{
    if (m_material) {
        auto& color = m_material->GetProperties().baseColor;
        return XMVectorSet(color.x, color.y, color.z, color.w);
    }
    return m_color;
}

// 材质相关方法
void RenderComponent::SetMaterial(std::shared_ptr<Engine::Material> material)
{
    m_material = material;
    if (m_material) {
        // 同步颜色到材质
        XMFLOAT4 color;
        XMStoreFloat4(&color, m_color);
        m_material->SetBaseColor(color.x, color.y, color.z, color.w);
    }
}

std::shared_ptr<Engine::Material> RenderComponent::GetOrCreateMaterial()
{
    if (!m_material) {
        m_material = Engine::Material::CreateDefault();
        // 同步颜色到新材质
        XMFLOAT4 color;
        XMStoreFloat4(&color, m_color);
        m_material->SetBaseColor(color.x, color.y, color.z, color.w);
    }
    return m_material;
}

void RenderComponent::Initialize()
{
    LOG_DEBUG("RenderComponent", "RenderComponent initialized for GameObject: {0}", m_owner ? m_owner->name : "Unknown");
}

void RenderComponent::Update(float deltaTime)
{
    // 渲染组件通常不需要每帧更新，除非有动画等
}