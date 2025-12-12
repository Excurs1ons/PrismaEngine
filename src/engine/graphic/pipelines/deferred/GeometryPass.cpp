#include "GeometryPass.h"
#include "SceneManager.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "Material.h"
#include "Logger.h"
#include <algorithm>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

GeometryPass::GeometryPass()
{
    LOG_DEBUG("GeometryPass", "几何通道构造函数被调用");
    m_stats = {};
}

GeometryPass::~GeometryPass()
{
    LOG_DEBUG("GeometryPass", "几何通道析构函数被调用");
}

void GeometryPass::Execute(RenderCommandContext* context)
{
    if (!context || !m_gbuffer) {
        LOG_ERROR("GeometryPass", "无效的上下文或G-Buffer");
        return;
    }

    LOG_DEBUG("GeometryPass", "开始执行几何通道");
    m_stats = {};

    try {
        // 设置G-Buffer为渲染目标
        m_gbuffer->SetAsRenderTarget(context);

        // 设置视口
        context->SetViewport(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));

        // 清除G-Buffer
        m_gbuffer->Clear(context);

        // 设置着色器常量
        context->SetConstantBuffer("View", m_view);
        context->SetConstantBuffer("Projection", m_projection);
        context->SetConstantBuffer("ViewProjection", m_viewProjection);

        // 获取场景
        auto scene = SceneManager::GetInstance()->GetCurrentScene();
        if (!scene) {
            LOG_WARNING("GeometryPass", "没有活动场景");
            return;
        }

        // 获取相机
        auto camera = scene->GetMainCamera();
        if (!camera) {
            LOG_WARNING("GeometryPass", "没有主相机");
            return;
        }

        // 收集所有需要渲染的对象
        struct RenderObject {
            MeshRenderer* meshRenderer;
            DirectX::XMMATRIX worldMatrix;
            float distanceToCamera;
        };
        std::vector<RenderObject> renderObjects;

        // 获取相机位置
        DirectX::XMMATRIX cameraWorld = DirectX::XMMatrixInverse(camera->GetViewMatrix(), nullptr);
        DirectX::XMFLOAT3 cameraPos = DirectX::XMFLOAT3(
            DirectX::XMVectorGetX(cameraWorld.r[3]),
            DirectX::XMVectorGetY(cameraWorld.r[3]),
            DirectX::XMVectorGetZ(cameraWorld.r[3])
        );

        // 遍历场景中的所有游戏对象
        const auto& gameObjects = scene->GetGameObjects();
        for (const auto& gameObject : gameObjects) {
            if (!gameObject) continue;

            auto meshRenderer = gameObject->GetComponent<MeshRenderer>();
            if (!meshRenderer) continue;

            auto mesh = meshRenderer->GetMesh();
            if (!mesh || mesh->GetVertices().empty()) continue;

            // 跳过透明物体（透明物体在单独的通道渲染）
            auto material = meshRenderer->GetMaterial();
            if (material && material->alpha < 1.0f) continue;

            auto transform = gameObject->GetTransform();
            DirectX::XMMATRIX worldMatrix = transform ? transform->GetWorldMatrix() : DirectX::XMMatrixIdentity();

            // 计算到相机的距离（用于优化渲染顺序）
            DirectX::XMFLOAT3 objectPos = DirectX::XMFLOAT3(
                DirectX::XMVectorGetX(worldMatrix.r[3]),
                DirectX::XMVectorGetY(worldMatrix.r[3]),
                DirectX::XMVectorGetZ(worldMatrix.r[3])
            );
            float distance = sqrtf(
                powf(objectPos.x - cameraPos.x, 2) +
                powf(objectPos.y - cameraPos.y, 2) +
                powf(objectPos.z - cameraPos.z, 2)
            );

            renderObjects.push_back({
                meshRenderer,
                worldMatrix,
                distance
            });
        }

        // 根据距离排序（从远到近，利用深度测试优化）
        std::sort(renderObjects.begin(), renderObjects.end(),
                  [](const RenderObject& a, const RenderObject& b) {
                      return a.distanceToCamera > b.distanceToCamera;
                  });

        // 渲染所有对象
        for (const auto& renderObj : renderObjects) {
            auto meshRenderer = renderObj.meshRenderer;
            auto mesh = meshRenderer->GetMesh();
            auto material = meshRenderer->GetMaterial();

            // 设置世界矩阵
            context->SetConstantBuffer("World", renderObj.worldMatrix);

            // 计算法线矩阵（世界矩阵的逆转置）
            DirectX::XMMATRIX worldInverseTranspose = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(renderObj.worldMatrix, nullptr));
            context->SetConstantBuffer("WorldInverseTranspose", worldInverseTranspose);

            // 设置材质参数
            if (material) {
                context->SetConstantBuffer("MaterialAlbedo", &material->albedo, sizeof(DirectX::XMFLOAT3));
                context->SetConstantBuffer("MaterialMetallic", &material->metallic, sizeof(float));
                context->SetConstantBuffer("MaterialRoughness", &material->roughness, sizeof(float));
                context->SetConstantBuffer("MaterialAO", &material->ao, sizeof(float));
                context->SetConstantBuffer("MaterialEmissive", &material->emissive, sizeof(DirectX::XMFLOAT3));

                // 绑定纹理
                if (material->albedoTexture) {
                    context->SetShaderResource("AlbedoTexture", material->albedoTexture);
                }
                if (material->normalTexture) {
                    context->SetShaderResource("NormalTexture", material->normalTexture);
                }
                if (material->metallicTexture) {
                    context->SetShaderResource("MetallicTexture", material->metallicTexture);
                }
                if (material->roughnessTexture) {
                    context->SetShaderResource("RoughnessTexture", material->roughnessTexture);
                }
                if (material->aoTexture) {
                    context->SetShaderResource("AOTexture", material->aoTexture);
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

                // 绘制
                context->DrawIndexed(static_cast<uint32_t>(indices.size()));

                // 更新统计
                m_stats.renderedObjects++;
                m_stats.triangles += static_cast<uint32_t>(indices.size() / 3);
            }
        }

        LOG_DEBUG("GeometryPass", "几何通道完成 - 渲染对象: {0}, 三角形: {1}",
                  m_stats.renderedObjects, m_stats.triangles);
    }
    catch (const std::exception& e) {
        LOG_ERROR("GeometryPass", "执行异常: {0}", e.what());
    }
}

void GeometryPass::SetGBuffer(std::shared_ptr<GBuffer> gbuffer)
{
    m_gbuffer = gbuffer;
    LOG_DEBUG("GeometryPass", "设置G-Buffer: 0x{0:x}",
              reinterpret_cast<uintptr_t>(gbuffer.get()));
}

void GeometryPass::ClearRenderTarget(float r, float g, float b, float a)
{
    // G-Buffer的清除由GBuffer类处理
    LOG_DEBUG("GeometryPass", "清除G-Buffer委托给GBuffer类");
}

void GeometryPass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    LOG_DEBUG("GeometryPass", "设置视口: {0}x{1}", width, height);

    // 调整G-Buffer尺寸
    if (m_gbuffer) {
        m_gbuffer->Resize(width, height);
    }
}

void GeometryPass::SetViewMatrix(const DirectX::XMMATRIX& view)
{
    m_view = view;
    m_viewProjection = view * m_projection;
}

void GeometryPass::SetProjectionMatrix(const DirectX::XMMATRIX& projection)
{
    m_projection = projection;
    m_viewProjection = m_view * projection;
}

void GeometryPass::SetViewProjectionMatrix(const DirectX::XMMATRIX& viewProjection)
{
    m_viewProjection = viewProjection;
}

void GeometryPass::SetDepthPrePass(bool enable)
{
    m_depthPrePass = enable;
    LOG_DEBUG("GeometryPass", "深度预渲染: {0}", enable ? "启用" : "禁用");
}

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine