#include "MeshRenderer.h"
#include "Transform.h"
#include "GameObject.h"
#include "Logger.h"
#include "RenderCommandContext.h"
#include <cassert>
#include <cmath>

namespace Prisma::Graphic {

MeshRenderer::MeshRenderer() {}

MeshRenderer::~MeshRenderer() {}

void MeshRenderer::DrawMesh(RenderCommandContext* context, std::shared_ptr<Mesh> mesh)
{
    if (!mesh || !context) {
        return;
    }

    for (const auto& subMesh : mesh->GetSubMeshes()) {
        if (subMesh.indexCount > 0) {
            context->DrawIndexed(subMesh.indexCount, 0, 0);
        } else if (subMesh.vertexCount > 0) {
            context->Draw(subMesh.vertexCount, 0);
        }
    }
}

void MeshRenderer::Render(RenderCommandContext* context)
{
    if (!m_mesh || !context) {
        return;
    }

    if (auto owner = GetOwner()) {
        if (auto transform = owner->GetTransform()) {
            Prisma::Matrix4x4 matrix = transform->GetMatrix();
            context->SetConstantBuffer("ObjectConstants", reinterpret_cast<const float*>(&matrix), 16);
        }
    }

    DrawMesh(context, m_mesh);
}

void MeshRenderer::Update(Timestep ts) {
    if (m_material && GetOwner() && GetOwner()->GetTransform()) {
        const auto position = GetOwner()->GetTransform()->GetPosition();
        const float pulse = 0.5f + 0.5f * std::sin(ts.GetSeconds());
        m_material->SetParam("ObjectPosition", PrismaMath::vec3(position.x, position.y, position.z));
        m_material->SetParam("FramePulse", pulse);
    }
}

void MeshRenderer::Initialize() {
    if (!GetOwner()) {
        LOG_ERROR("Renderer", "MeshRenderer initialized without an owner!");
        return;
    }
    
    if (!GetOwner()->GetTransform()) {
        LOG_WARNING("Renderer", "MeshRenderer owner '{0}' has no Transform component. Rendering may fail.", GetOwner()->name);
    }
    
    LOG_TRACE("Renderer", "MeshRenderer initialized for object '{0}'", GetOwner()->name);
}

void MeshRenderer::Shutdown() {
    LOG_TRACE("Renderer", "MeshRenderer shutting down for object '{0}'", GetOwner() ? GetOwner()->name : "Unknown");
    m_mesh.reset();
    m_material.reset();
}

} // namespace Prisma::Graphic
