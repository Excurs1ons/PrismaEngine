#include "RenderComponent.h"
#include "RenderCommandContext.h"

#include <utility>
#include "Material.h"
#include "Transform.h"
#include "Logger.h"

using PrismaEngine::Graphic::RenderCommandContext;

RenderComponent::RenderComponent()
    : m_vertexCount(0)
    , m_indexCount(0)
    , m_use16BitIndices(true) // 默认使用16位索引
    , m_color(1.0f, 1.0f, 1.0f, 1.0f) // 默认白色
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
    LOG_DEBUG("RenderComponent", "Render called - vertexCount={0}, indexCount={1}", m_vertexCount, m_indexCount);

    if (!context || m_vertexCount == 0)
    {
        LOG_WARNING("RenderComponent", "Render failed - context={0}, vertexCount={1}",
                   context ? "valid" : "null", m_vertexCount);
        return;
    }

    // 应用材质 (这会设置颜色、纹理等参数)
    auto material = GetOrCreateMaterial();
    if (material) {
        LOG_DEBUG("RenderComponent", "应用材质");
        material->Apply(context);
    }

    // 设置世界矩阵 (寄存器 b1)
    if (auto* transform = m_owner->transform()) {
        // 手动构建世界矩阵：位置 * 旋转 * 缩放
        Prisma::Matrix4x4 translation = Prisma::Math::Translation(transform->position);
        Prisma::Matrix4x4 rotationX = Prisma::Math::RotationX(transform->rotation.x);
        Prisma::Matrix4x4 rotationY = Prisma::Math::RotationY(transform->rotation.y);
        Prisma::Matrix4x4 rotationZ = Prisma::Math::RotationZ(transform->rotation.z);
        Prisma::Matrix4x4 scale = Prisma::Math::Scale(transform->scale);

        // 组合矩阵：S * R * T
        Prisma::Matrix4x4 worldMatrix = scale * rotationZ * rotationY * rotationX * translation;

        context->SetConstantBuffer("World", reinterpret_cast<const float*>(&worldMatrix), 16);
    }

    // 绑定顶点缓冲区到渲染后端（将数据复制到后端的 per-frame upload buffer）
    // 顶点布局: 7 floats per vertex (x,y,z,r,g,b,a)
    const uint32_t stride = 7 * sizeof(float);
    const uint32_t vertexSizeInBytes = m_vertexCount * stride;
    LOG_DEBUG("RenderComponent", "设置顶点缓冲区: {0} 个顶点, 总大小 {1} 字节, stride={2}",
               m_vertexCount, vertexSizeInBytes, stride);
    context->SetVertexBuffer(m_vertices.data(), vertexSizeInBytes, stride);

    // 如果有索引数据，绑定索引缓冲区
    if (m_indexCount > 0) {
        LOG_DEBUG("RenderComponent", "设置索引缓冲区: {0} 个索引, 16位={1}", m_indexCount, m_use16BitIndices);
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
        LOG_DEBUG("RenderComponent", "执行索引绘制: {0} 个索引", m_indexCount);
        context->DrawIndexed(m_indexCount);
    } else {
        // 执行普通顶点绘制
        LOG_DEBUG("RenderComponent", "执行顶点绘制: {0} 个顶点", m_vertexCount);
        context->Draw(m_vertexCount);
    }

    LOG_DEBUG("RenderComponent", "Render completed");
}

void RenderComponent::SetColor(float r, float g, float b, float a)
{
    m_color = Prisma::Color(r, g, b, a);

    // 如果已有材质，更新其基础颜色
    if (m_material) {
        m_material->SetBaseColor(r, g, b, a);
    }
}

// 获取颜色
Prisma::Color RenderComponent::GetColor() const
{
    if (m_material) {
        const auto& color = m_material->GetProperties().baseColor;
        return color;
    }
    return m_color;
}

// 材质相关方法
void RenderComponent::SetMaterial(std::shared_ptr<Engine::Material> material)
{
    m_material = std::move(material);
    if (m_material) {
        // 同步颜色到材质
        m_material->SetBaseColor(m_color);
    }
}

std::shared_ptr<Engine::Material> RenderComponent::GetOrCreateMaterial()
{
    if (!m_material) {
        m_material = Engine::Material::CreateDefault();
        // 同步颜色到新材质
        Prisma::Color color;
        m_material->SetBaseColor(m_color);
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