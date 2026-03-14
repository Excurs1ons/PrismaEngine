#include "GeometryRenderPass.h"
#include "Mesh.h"
#include "Logger.h"
#include "RenderCommandContext.h"
#include "interfaces/IBuffer.h"

using Prisma::Graphic::RenderCommandContext;

namespace Prisma {

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

void GeometryRenderPass::Execute(Prisma::Graphic::RenderCommandContext* context)
{
    if (!context) {
        LOG_WARNING("GeometryRenderPass", "Render command context is null");
        return;
    }
    
    ClearRenderTarget(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
    
    for (const auto& item : m_renderQueue) {
        if (!item.mesh) {
            continue;
        }
        
        for (const auto& subMesh : item.mesh->subMeshes) {
            if (subMesh.vertexBufferHandle.IsValid()) {
                context->SetVertexBuffer(
                    reinterpret_cast<IBuffer*>(subMesh.vertexBufferHandle.GetId()),
                    0, 0, sizeof(Vertex)
                );
            }
            
            if (subMesh.indexBufferHandle.IsValid()) {
                context->SetIndexBuffer(
                    reinterpret_cast<IBuffer*>(subMesh.indexBufferHandle.GetId()),
                    0, true
                );
            }
            
            context->SetConstantData(0, item.transform, sizeof(item.transform));
            
            if (subMesh.indexBufferHandle.IsValid() && subMesh.indicesCount() > 0) {
                context->DrawIndexed(subMesh.indicesCount(), 0);
            } else if (subMesh.verticesCount() > 0) {
                context->Draw(subMesh.verticesCount(), 0);
            }
        }
        
        LOG_DEBUG("GeometryRenderPass", "Rendered mesh with {0} submeshes", 
                  item.mesh->subMeshes.size());
    }
    
    LOG_INFO("GeometryRenderPass", "Executed geometry render pass with {0} meshes", 
             m_renderQueue.size());
}

void GeometryRenderPass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
    LOG_DEBUG("GeometryRenderPass", "Render target set");
}

void GeometryRenderPass::ClearRenderTarget(float r, float g, float b, float a)
{
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
    
    LOG_DEBUG("GeometryRenderPass", "Clear color set to ({0}, {1}, {2}, {3})", r, g, b, a);
}

void GeometryRenderPass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    
    LOG_DEBUG("GeometryRenderPass", "Viewport set to {0}x{1}", width, height);
}

void GeometryRenderPass::AddMeshToRenderQueue(std::shared_ptr<Mesh> mesh, const float* transform)
{
    if (!mesh) {
        LOG_WARNING("GeometryRenderPass", "Trying to add null mesh to render queue");
        return;
    }
    
    if (!transform) {
        LOG_WARNING("GeometryRenderPass", "Trying to add mesh with null transform");
        return;
    }
    
    RenderItem item;
    item.mesh = mesh;
    
    // 复制变换矩阵
    for (int i = 0; i < 16; i++) {
        item.transform[i] = transform[i];
    }
    
    m_renderQueue.push_back(item);
    LOG_DEBUG("GeometryRenderPass", "Mesh added to render queue. Total items: {0}", 
              m_renderQueue.size());
}

} // namespace Engine