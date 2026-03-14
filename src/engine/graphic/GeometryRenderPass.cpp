#include "GeometryRenderPass.h"
#include "Mesh.h"
#include "Logger.h"
#include "RenderCommandContext.h"
#include "interfaces/IBuffer.h"

namespace Prisma::Graphic {

GeometryRenderPass::GeometryRenderPass()
    : m_renderTarget(nullptr)
    , m_width(0)
    , m_height(0)
{
    m_clearColor[0] = 0.0f;
    m_clearColor[1] = 0.0f;
    m_clearColor[2] = 0.0f;
    m_clearColor[3] = 1.0f;
}

GeometryRenderPass::~GeometryRenderPass()
{
}

void GeometryRenderPass::Execute(RenderCommandContext* context)
{
    if (!context) {
        LOG_WARNING("GeometryRenderPass", "Render command context is null");
        return;
    }
    
    // ClearRenderTarget logic here if needed
    
    for (const auto& item : m_renderQueue) {
        if (!item.mesh) {
            continue;
        }
        
        for (const auto& subMesh : item.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer) {
                context->SetVertexBuffer(
                    subMesh.vertexBuffer.get(),
                    0, 0, sizeof(Vertex)
                );
            }
            
            if (subMesh.indexBuffer) {
                context->SetIndexBuffer(
                    subMesh.indexBuffer.get(),
                    0, subMesh.use16BitIndices
                );
            }
            
            context->SetConstantData(0, item.transform, sizeof(item.transform));
            
            if (subMesh.indexBuffer && subMesh.indexCount > 0) {
                context->DrawIndexed(subMesh.indexCount, 0);
            } else if (subMesh.vertexCount > 0) {
                context->Draw(subMesh.vertexCount, 0);
            }
        }
    }
}

void GeometryRenderPass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
}

void GeometryRenderPass::ClearRenderTarget(float r, float g, float b, float a)
{
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
}

void GeometryRenderPass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
}

void GeometryRenderPass::AddMeshToRenderQueue(std::shared_ptr<Mesh> mesh, const float* transform)
{
    if (!mesh || !transform) {
        return;
    }
    
    RenderItem item;
    item.mesh = mesh;
    for (int i = 0; i < 16; i++) {
        item.transform[i] = transform[i];
    }
    
    m_renderQueue.push_back(item);
}

} // namespace Prisma::Graphic
