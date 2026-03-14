#include "RenderComponent.h"
#include "RenderCommandContext.h"
#include "Material.h"
#include "Transform.h"
#include "Logger.h"
#include <utility>
#include <cstring>

namespace Prisma::Graphic {

RenderComponent::RenderComponent()
    : m_vertexCount(0), m_indexCount(0), m_use16BitIndices(true)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
{
}

void RenderComponent::SetVertexData(const float *vertices, uint32_t vertexCount) {
    m_vertices.clear();
    m_vertices.resize(vertexCount * 7); // 7 floats per vertex: pos(3) + col(4)
    std::memcpy(m_vertices.data(), vertices, vertexCount * 7 * sizeof(float));
    m_vertexCount = vertexCount;
}

void RenderComponent::SetIndexData(const uint32_t *indices, uint32_t indexCount) {
    m_indices.clear();
    m_indices.resize(indexCount);
    std::memcpy(m_indices.data(), indices, indexCount * sizeof(uint32_t));
    m_indexCount = indexCount;

    m_use16BitIndices = true;
    for (uint32_t i = 0; i < indexCount; ++i) {
        if (indices[i] > 65535) {
            m_use16BitIndices = false;
            break;
        }
    }
}

void RenderComponent::SetIndexData(const uint16_t *indices, uint32_t indexCount) {
    m_indices.clear();
    m_indices.resize(indexCount);
    for (uint32_t i = 0; i < indexCount; ++i) {
        m_indices[i] = static_cast<uint32_t>(indices[i]);
    }
    m_indexCount = indexCount;
    m_use16BitIndices = true;
}

void RenderComponent::Render(RenderCommandContext *context) {
    if (!context || m_vertexCount == 0) {
        return;
    }

    // Bind Material
    auto material = GetOrCreateMaterial();
    if (material) {
        material->Bind(reinterpret_cast<ICommandBuffer*>(context)); // Hack for now
    }

    // Set World Matrix
    if (auto transform = GetOwner()->GetTransform()) {
        Prisma::Matrix4x4 worldMatrix = transform->GetMatrix();
        context->SetConstantBuffer("World", reinterpret_cast<const float *>(&worldMatrix), 16);
    }

    const uint32_t stride = 7 * sizeof(float);
    const uint32_t vertexSizeInBytes = m_vertexCount * stride;
    context->SetVertexBuffer(m_vertices.data(), vertexSizeInBytes, stride);

    if (m_indexCount > 0) {
        if (m_use16BitIndices) {
            std::vector<uint16_t> indices16(m_indexCount);
            for (uint32_t i = 0; i < m_indexCount; ++i) {
                indices16[i] = static_cast<uint16_t>(m_indices[i]);
            }
            context->SetIndexBuffer(indices16.data(), m_indexCount * sizeof(uint16_t), true);
        } else {
            context->SetIndexBuffer(m_indices.data(), m_indexCount * sizeof(uint32_t), false);
        }
        context->DrawIndexed(m_indexCount);
    } else {
        context->Draw(m_vertexCount);
    }
}

void RenderComponent::SetColor(float r, float g, float b, float a) {
    m_color = Prisma::Color(r, g, b, a);
    if (m_material) {
        m_material->SetBaseColor(m_color);
    }
}

PrismaMath::vec4 RenderComponent::GetColor() const {
    return m_color;
}

void RenderComponent::SetMaterial(std::shared_ptr<Material> material) {
    m_material = std::move(material);
    if (m_material) {
        m_material->SetBaseColor(m_color);
    }
}

std::shared_ptr<Material> RenderComponent::GetOrCreateMaterial() {
    if (!m_material) {
        m_material = Material::CreateDefault();
        m_material->SetBaseColor(m_color);
    }
    return m_material;
}

void RenderComponent::Initialize() {
    LOG_DEBUG("RenderComponent", "RenderComponent initialized for GameObject: {0}",
              GetOwner() ? GetOwner()->name : "Unknown");
}

void RenderComponent::Update(Timestep ts) {
}

void RenderComponent::Shutdown() {
    Component::Shutdown();
}

} // namespace Prisma::Graphic
