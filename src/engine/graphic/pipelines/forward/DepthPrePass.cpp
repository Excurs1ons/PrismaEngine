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
            if (!mesh) continue;

            // 设置世界矩阵
            auto transform = gameObject->GetTransform();
            if (transform) {
                XMMATRIX worldMatrix = transform->GetWorldMatrix();
                context->SetConstantBuffer("World", worldMatrix);
            }

            // 设置顶点和索引缓冲区
            const auto& vertices = mesh->GetVertices();
            const auto& indices = mesh->GetIndices();

            if (!vertices.empty()) {
                context->SetVertexBuffer(vertices.data(),
                                      static_cast<uint32_t>(vertices.size() * sizeof(float)),
                                      mesh->GetVertexStride());
            }

            if (!indices.empty()) {
                context->SetIndexBuffer(indices.data(),
                                      static_cast<uint32_t>(indices.size() * sizeof(uint32_t)),
                                      false); // 32位索引
            }

            // 深度预渲染 - 只写入深度，不进行颜色计算
            context->DrawIndexed(static_cast<uint32_t>(indices.size()));
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

    // 设置视口到渲染上下文
    context->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
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