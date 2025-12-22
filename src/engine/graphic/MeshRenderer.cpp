#include "MeshRenderer.h"
#include "Transform.h"
#include "GameObject.h"
#include "Logger.h"
#include <cassert>

void MeshRenderer::DrawMesh(RenderCommandContext* context, std::shared_ptr<Mesh> mesh)
{
    LOG_DEBUG("MeshRenderer", "DrawMesh called. mesh ptr={0}, subMeshes={1}", reinterpret_cast<uintptr_t>(mesh.get()), mesh ? mesh->subMeshes.size() : 0);

    if (!mesh) {
        LOG_WARNING("MeshRenderer", "DrawMesh: mesh is null");
        assert(mesh && "DrawMesh: mesh is null");
        return;
    }
    if (!context) {
        LOG_WARNING("MeshRenderer", "DrawMesh: render context is null");
        assert(context && "DrawMesh: render context is null");
        return;
    }

    // 遍历所有子网格并绘制
    for (size_t i = 0; i < mesh->subMeshes.size(); ++i) {
        const auto& subMesh = mesh->subMeshes[i];
        LOG_DEBUG("MeshRenderer", "SubMesh[{0}] name='{1}' vertices={2} indices={3}", i, subMesh.name, subMesh.verticesCount(), subMesh.indicesCount());

        if (subMesh.verticesCount() == 0) {
            LOG_WARNING("MeshRenderer", "SubMesh[{0}] has no vertices. Skipping draw.", i);
            continue;
        }

        if (subMesh.indicesCount() > 0) {
            // If indices exist, attempt indexed draw
            LOG_TRACE("MeshRenderer", "Calling DrawIndexed for SubMesh[{0}] indexCount={1}", i, subMesh.indicesCount());
            context->DrawIndexed(subMesh.indicesCount(), 0, 0);
        } else {
            // Fallback to non-indexed draw (use renderer's currently bound vertex buffer)
            LOG_TRACE("MeshRenderer", "Calling Draw (non-indexed) for SubMesh[{0}] vertexCount={1}", i, subMesh.verticesCount());
            context->Draw(subMesh.verticesCount(), 0);
        }
    }
}

// 将内联实现移出到此处
void MeshRenderer::Render(RenderCommandContext* context)
{
    LOG_DEBUG("MeshRenderer", "Render called. mesh present={0} material present={1} context ptr={2}", (bool)m_mesh, (bool)m_material, reinterpret_cast<uintptr_t>(context));

    // 断言以便在调试模式下尽早发现问题
    assert(context && "MeshRenderer::Render: context is null");

    if (!m_mesh) {
        LOG_WARNING("MeshRenderer", "Render: m_mesh is null");
        return;
    }
    if (!context) {
        LOG_ERROR("MeshRenderer", "Render: context is null, cannot render");
        return;
    }

    if (!m_material) {
        LOG_WARNING("MeshRenderer", "Render: m_material is null - proceeding with default pipeline state");
    }

    // 设置材质和管线状态
    // m_material->Apply(context); // 需要实现Material::Apply方法

    // 设置物体变换
    if (m_owner) {
        Transform* transform = m_owner->transform();
        if (!transform) {
            LOG_WARNING("MeshRenderer", "Render: owner has no Transform");
            assert(transform && "MeshRenderer::Render: owner transform is null");
        } else {
            LOG_DEBUG("MeshRenderer", "Render: setting ObjectConstants from Transform ptr={0}", reinterpret_cast<uintptr_t>(transform));
            context->SetConstantBuffer("ObjectConstants", transform->GetMatrix());
        }
    } else {
        LOG_WARNING("MeshRenderer", "Render: m_owner is null");
        assert(m_owner && "MeshRenderer::Render: owner is null");
    }

    // 绘制网格
    DrawMesh(context, m_mesh);
}

void MeshRenderer::Update(float deltaTime) {}

MeshRenderer::MeshRenderer() {}

MeshRenderer::~MeshRenderer() {}

void MeshRenderer::Initialize() {}

void MeshRenderer::Shutdown() {}
