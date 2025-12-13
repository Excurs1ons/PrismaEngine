#include "DepthPrePass.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include <DirectXColors.h>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

DepthPrePass::DepthPrePass()
{
    LOG_DEBUG("DepthPrePass", "构造函数被调用");
}

DepthPrePass::~DepthPrePass()
{
    LOG_DEBUG("DepthPrePass", "析构函数被调用");
}

void DepthPrePass::Execute(RenderCommandContext* context)
{
    if (!context || !m_depthBuffer) {
        LOG_WARNING("DepthPrePass", "Invalid context or depth buffer");
        return;
    }

    LOG_DEBUG("DepthPrePass", "执行深度预渲染");

    try {
        // 设置深度-only着色器参数
        context->SetConstantBuffer("ViewProjection", m_viewProjection);

        // 获取场景
        auto scene = SceneManager::GetInstance()->GetCurrentScene();
        if (!scene) {
            LOG_WARNING("DepthPrePass", "No active scene");
            return;
        }

        // 遍历所有带有MeshRenderer的游戏对象
        const auto& gameObjects = scene->GetGameObjects();

        for (const auto& gameObject : gameObjects) {
            if (!gameObject) continue;

            auto meshRenderer = gameObject->GetComponent<MeshRenderer>();
            if (!meshRenderer) continue;

            // 获取网格数据
            auto mesh = meshRenderer->GetMesh();
            if (!mesh || mesh->subMeshes.empty()) continue;

            // 设置世界矩阵
            auto transform = gameObject->transform();
            if (transform) {
                // Transform::GetMatrix() 返回的是转置后的矩阵
                float* matrixData = transform->GetMatrix();
                XMMATRIX worldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(matrixData));
                // 再次转置回来，因为GetMatrix()为了HLSL已经转置了一次
                worldMatrix = XMMatrixTranspose(worldMatrix);
                context->SetConstantBuffer("World", worldMatrix);
            }

            // 渲染所有子网格
            for (const auto& subMesh : mesh->subMeshes) {
                if (!subMesh.vertexBufferHandle) continue; // 如果没有上传到GPU则跳过

                // 使用RenderBackend来渲染
                // 这里简化处理，直接渲染
                context->SetVertexBuffer(nullptr, 0, subMesh.GetVertexStride());
                context->SetIndexBuffer(nullptr, 0, false);

                // 绘制子网格
                context->DrawIndexed(subMesh.indicesCount());
            }
        }

        LOG_DEBUG("DepthPrePass", "深度预渲染完成");
    }
    catch (const std::exception& e) {
        LOG_ERROR("DepthPrePass", "Exception during execution: {0}", e.what());
    }
}

void DepthPrePass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
    LOG_DEBUG("DepthPrePass", "设置渲染目标: 0x{0:x}", reinterpret_cast<uintptr_t>(renderTarget));
}

void DepthPrePass::ClearRenderTarget(float r, float g, float b, float a)
{
    // 深度预渲染只需要清除深度缓冲
    LOG_DEBUG("DepthPrePass", "清除深度缓冲");

    if (m_depthBuffer) {
        // 设置深度清除值为1.0（最远）
        // 这里应该调用具体的图形API来清除深度缓冲
        // context->ClearDepthBuffer(1.0f);
    }
}

void DepthPrePass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    LOG_DEBUG("DepthPrePass", "设置视口: {0}x{1}", width, height);
}

void DepthPrePass::SetDepthBuffer(void* depthBuffer)
{
    m_depthBuffer = depthBuffer;
    LOG_DEBUG("DepthPrePass", "设置深度缓冲区: 0x{0:x}", reinterpret_cast<uintptr_t>(depthBuffer));
}

void DepthPrePass::SetViewProjectionMatrix(const XMMATRIX& viewProjection)
{
    m_viewProjection = viewProjection;
}

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine