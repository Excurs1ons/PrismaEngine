#include "TransparentPass.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "Material.h"
#include <DirectXColors.h>
#include <algorithm>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

TransparentPass::TransparentPass()
    : m_depthWrite(false)
    , m_context(nullptr)
{
    LOG_DEBUG("TransparentPass", "构造函数被调用");
}

TransparentPass::~TransparentPass()
{
    LOG_DEBUG("TransparentPass", "析构函数被调用");
}

void TransparentPass::Execute(RenderCommandContext* context)
{
    m_context = context;

    if (!context || !m_renderTarget || !m_depthBuffer) {
        LOG_WARNING("TransparentPass", "Invalid context, render target or depth buffer");
        return;
    }

    LOG_DEBUG("TransparentPass", "开始渲染透明物体");

    try {
        // 计算视图投影矩阵
        m_viewProjection = m_view * m_projection;

        // 设置着色器常量
        context->SetConstantBuffer("View", m_view);
        context->SetConstantBuffer("Projection", m_projection);
        context->SetConstantBuffer("ViewProjection", m_viewProjection);
        context->SetConstantBuffer("DepthWrite", &m_depthWrite, sizeof(bool));

        // 获取场景
        auto scene = SceneManager::GetInstance()->GetCurrentScene();
        if (!scene) {
            LOG_WARNING("TransparentPass", "No active scene");
            return;
        }

        // 获取主相机
        auto camera = scene->GetMainCamera();
        XMFLOAT3 cameraPos = XMFLOAT3(0, 0, 0);
        if (camera) {
            XMMATRIX cameraWorld = XMMatrixInverse(camera->GetViewMatrix(), nullptr);
            cameraPos = XMFLOAT3(
                XMVectorGetX(cameraWorld.r[3]),
                XMVectorGetY(cameraWorld.r[3]),
                XMVectorGetZ(cameraWorld.r[3])
            );
        }

        // 收集所有透明对象
        struct TransparentObject {
            MeshRenderer* meshRenderer;
            XMMATRIX worldMatrix;
            float distanceToCamera;
            float alpha;
        };
        std::vector<TransparentObject> transparentObjects;

        const auto& gameObjects = scene->GetGameObjects();
        for (const auto& gameObject : gameObjects) {
            if (!gameObject) continue;

            auto meshRenderer = gameObject->GetComponent<MeshRenderer>();
            if (!meshRenderer) continue;

            auto mesh = meshRenderer->GetMesh();
            if (!mesh || mesh->GetVertices().empty()) continue;

            auto material = meshRenderer->GetMaterial();
            if (!material || material->alpha <= 0.01f) continue; // 完全透明的物体跳过

            auto transform = gameObject->GetTransform();
            XMMATRIX worldMatrix = transform ? transform->GetWorldMatrix() : DirectX::XMMatrixIdentity();

            // 计算到相机的距离
            XMFLOAT3 objectPos = XMFLOAT3(
                XMVectorGetX(worldMatrix.r[3]),
                XMVectorGetY(worldMatrix.r[3]),
                XMVectorGetZ(worldMatrix.r[3])
            );
            float distance = sqrtf(
                powf(objectPos.x - cameraPos.x, 2) +
                powf(objectPos.y - cameraPos.y, 2) +
                powf(objectPos.z - cameraPos.z, 2)
            );

            transparentObjects.push_back({
                meshRenderer,
                worldMatrix,
                distance,
                material->alpha
            });
        }

        // 透明物体需要从近到远排序（保证正确的Alpha混合）
        std::sort(transparentObjects.begin(), transparentObjects.end(),
                  [](const TransparentObject& a, const TransparentObject& b) {
                      return a.distanceToCamera < b.distanceToCamera;
                  });

        // 渲染所有透明物体
        for (const auto& transObj : transparentObjects) {
            auto meshRenderer = transObj.meshRenderer;
            auto mesh = meshRenderer->GetMesh();
            auto material = meshRenderer->GetMaterial();

            // 设置世界矩阵
            context->SetConstantBuffer("World", transObj.worldMatrix);

            // 设置材质参数（包括Alpha）
            if (material) {
                XMFLOAT4 albedoWithAlpha = {
                    material->albedo.x,
                    material->albedo.y,
                    material->albedo.z,
                    transObj.alpha
                };
                context->SetConstantBuffer("MaterialAlbedo", &albedoWithAlpha, sizeof(XMFLOAT4));
                context->SetConstantBuffer("MaterialMetallic", &material->metallic, sizeof(float));
                context->SetConstantBuffer("MaterialRoughness", &material->roughness, sizeof(float));
                context->SetConstantBuffer("MaterialAO", &material->ao, sizeof(float));

                // 绑定纹理
                if (material->albedoTexture) {
                    context->SetShaderResource("AlbedoTexture", material->albedoTexture);
                }
                if (material->normalTexture) {
                    context->SetShaderResource("NormalTexture", material->normalTexture);
                }
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
                                      false);

                // 启用Alpha混合
                context->SetBlendState(true);

                // 绘制
                context->DrawIndexed(static_cast<uint32_t>(indices.size()));

                // 禁用Alpha混合（避免影响后续绘制）
                context->SetBlendState(false);
            }
        }

        LOG_DEBUG("TransparentPass", "透明物体渲染完成，共 {0} 个", transparentObjects.size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("TransparentPass", "Exception during execution: {0}", e.what());
    }
}

void TransparentPass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
    LOG_DEBUG("TransparentPass", "设置渲染目标: 0x{0:x}", reinterpret_cast<uintptr_t>(renderTarget));
}

void TransparentPass::ClearRenderTarget(float r, float g, float b, float a)
{
    LOG_DEBUG("TransparentPass", "清除颜色缓冲: ({0}, {1}, {2}, {3})", r, g, b, a);
}

void TransparentPass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    LOG_DEBUG("TransparentPass", "设置视口: {0}x{1}", width, height);
}

void TransparentPass::SetDepthBuffer(void* depthBuffer)
{
    m_depthBuffer = depthBuffer;
    LOG_DEBUG("TransparentPass", "设置深度缓冲区（只读）: 0x{0:x}", reinterpret_cast<uintptr_t>(depthBuffer));
}

void TransparentPass::SetViewMatrix(const XMMATRIX& view)
{
    m_view = view;
}

void TransparentPass::SetProjectionMatrix(const XMMATRIX& projection)
{
    m_projection = projection;
}

void TransparentPass::SetDepthWrite(bool enable)
{
    m_depthWrite = enable;
    LOG_DEBUG("TransparentPass", "设置深度写入: {0}", enable ? "启用" : "禁用");
}

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine