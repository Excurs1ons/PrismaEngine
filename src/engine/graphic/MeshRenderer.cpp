#include "MeshRenderer.h"
#include "Transform.h"
#include "GameObject.h"
#include "Logger.h"
#include "RenderCommandContext.h"
#include <cassert>

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
            Prisma::Matrix4x4 matrix = transform->GetWorldMatrix();
            context->SetConstantBuffer("ObjectConstants", reinterpret_cast<const float*>(&matrix), 16);
        }
    }

    DrawMesh(context, m_mesh);
}

void MeshRenderer::Update(Timestep ts) {}

void MeshRenderer::Initialize() {}

void MeshRenderer::Shutdown() {}

} // namespace Prisma::Graphic
