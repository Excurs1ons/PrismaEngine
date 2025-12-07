#include "RenderPass.h"
#include "Mesh.h"

RenderPass::RenderPass()
{
}

RenderPass::~RenderPass()
{
}

RenderPass2D::RenderPass2D()
    : m_cameraMatrix(XMMatrixIdentity())
{
}

RenderPass2D::~RenderPass2D()
{
}

void RenderPass2D::Execute(RenderCommandContext* context)
{
    // 2D渲染通道执行逻辑
    // 在这里会实际执行所有排队的2D渲染命令
}

void RenderPass2D::AddMeshToRenderQueue(std::shared_ptr<Mesh> mesh, FXMMATRIX transform)
{
    // 将网格和变换添加到渲染队列中
    // 在实际实现中，我们会存储这些信息并在Execute时使用
}

void RenderPass2D::SetCameraMatrix(FXMMATRIX viewProjection)
{
    m_cameraMatrix = viewProjection;
}