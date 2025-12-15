#include "OpaquePass.h"
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

OpaquePass::OpaquePass()
{
    LOG_DEBUG("OpaquePass", "构造函数被调用");
    m_stats = {};
}

OpaquePass::~OpaquePass()
{
    LOG_DEBUG("OpaquePass", "析构函数被调用");
}

void OpaquePass::Execute(RenderCommandContext* context)
{
    if (!context || !m_renderTarget || !m_depthBuffer) {
        LOG_WARNING("OpaquePass", "Invalid context, render target or depth buffer");
        return;
    }

    LOG_DEBUG("OpaquePass", "开始渲染不透明物体");

    // 重置统计
    m_stats = {};

    try {
        // 计算视图投影矩阵
        m_viewProjection = m_view * m_projection;

        // 设置着色器常量
        LOG_DEBUG("OpaquePass", "设置ViewProjection矩阵");
        context->SetConstantBuffer("ViewProjection", m_viewProjection);
        // Note: 不单独设置View和Projection，因为默认着色器只使用ViewProjection
        // context->SetConstantBuffer("View", m_view);
        // context->SetConstantBuffer("Projection", m_projection);
        context->SetConstantBuffer("AmbientLight", reinterpret_cast<const float*>(&m_ambientLight), sizeof(XMFLOAT3));

        // 设置光源数据
        if (!m_lights.empty()) {
            context->SetConstantBuffer("Lights", reinterpret_cast<const float*>(m_lights.data()),
                                        static_cast<size_t>(m_lights.size() * sizeof(Light)));
            uint32_t lightCount = static_cast<uint32_t>(m_lights.size());
            context->SetConstantBuffer("LightCount", reinterpret_cast<const float*>(&lightCount), sizeof(uint32_t));
        }

        // 获取场景
        auto scene = SceneManager::GetInstance()->GetCurrentScene();
        if (!scene) {
            LOG_WARNING("OpaquePass", "No active scene");
            return;
        }

        // 获取主相机位置
        auto camera = scene->GetMainCamera();
        XMFLOAT3 cameraPos = XMFLOAT3(0, 0, 0);
        if (camera) {
            XMVECTOR pos = camera->GetPosition();
            cameraPos = XMFLOAT3(XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos));
        }

        // 遍历所有带有MeshRenderer的游戏对象
        const auto& gameObjects = scene->GetGameObjects();
        LOG_DEBUG("OpaquePass", "场景中总共有 {0} 个游戏对象", gameObjects.size());

        // 收集所有需要渲染的对象
        struct RenderObject {
            RenderComponent* renderComponent;
            MeshRenderer* meshRenderer;
            XMMATRIX worldMatrix;
            float distanceToCamera;
        };
        std::vector<RenderObject> renderObjects;

        for (const auto& gameObject : gameObjects) {
            if (!gameObject) continue;

            LOG_DEBUG("OpaquePass", "检查游戏对象: '{0}'", gameObject->name);

            // 优先检查RenderComponent
            auto renderComponent = gameObject->GetComponent<RenderComponent>();
            MeshRenderer* meshRenderer = nullptr;

            // 如果没有RenderComponent，尝试获取MeshRenderer（向后兼容）
            if (!renderComponent) {
                meshRenderer = gameObject->GetComponent<MeshRenderer>();
                if (!meshRenderer) {
                    LOG_DEBUG("OpaquePass", "  - 没有RenderComponent或MeshRenderer，跳过");
                    continue;
                }
                renderComponent = meshRenderer;
                LOG_DEBUG("OpaquePass", "  - 找到MeshRenderer");
            } else {
                // 如果有RenderComponent，检查它是否也是MeshRenderer
                meshRenderer = dynamic_cast<MeshRenderer*>(renderComponent);
                LOG_DEBUG("OpaquePass", "  - 找到RenderComponent");
            }

            // 检查是否有有效的网格数据
            if (meshRenderer) {
                auto mesh = meshRenderer->GetMesh();
                if (!mesh || mesh->subMeshes.empty()) {
                    LOG_DEBUG("OpaquePass", "  - MeshRenderer没有有效的网格数据，跳过");
                    continue;
                }
                LOG_DEBUG("OpaquePass", "  - MeshRenderer有 {0} 个子网格", mesh->subMeshes.size());
            } else {
                // 对于纯RenderComponent，检查顶点数据
                uint32_t vertexCount = renderComponent->GetVertexCount();
                if (vertexCount == 0) {
                    LOG_DEBUG("OpaquePass", "  - RenderComponent没有顶点数据，跳过");
                    continue;
                }
                LOG_DEBUG("OpaquePass", "  - RenderComponent有 {0} 个顶点", vertexCount);
            }

            auto transform = gameObject->transform();
            XMMATRIX worldMatrix;
            if (transform) {
                // Transform::GetMatrix() 返回的是标准的行主序矩阵
                float* matrixData = transform->GetMatrix();
                worldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(matrixData));
                // 转置矩阵以适应HLSL的列主序要求
                worldMatrix = XMMatrixTranspose(worldMatrix);
            } else {
                worldMatrix = DirectX::XMMatrixIdentity();
            }

            // 计算到相机的距离（用于排序）
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

            LOG_DEBUG("OpaquePass", "  - 添加对象到渲染列表，距离相机: {0}", distance);
            renderObjects.push_back({ renderComponent, meshRenderer, worldMatrix, distance });
            m_stats.objects++;
        }

        LOG_DEBUG("OpaquePass", "总共找到 {0} 个可渲染对象", renderObjects.size());

        // 按距离排序（从远到近，为了正确的深度缓冲和早期深度测试）
        std::sort(renderObjects.begin(), renderObjects.end(),
                  [](const RenderObject& a, const RenderObject& b) {
                      return a.distanceToCamera > b.distanceToCamera;
                  });

        // 渲染所有对象
        for (const auto& renderObj : renderObjects) {
            auto renderComponent = renderObj.renderComponent;

            // 设置世界矩阵
            context->SetConstantBuffer("World", renderObj.worldMatrix);

            // 如果是MeshRenderer，使用它的网格和材质
            if (renderObj.meshRenderer) {
                auto mesh = renderObj.meshRenderer->GetMesh();
                auto material = renderObj.meshRenderer->GetMaterial();

                // 设置材质参数
                if (material) {
                    const auto& props = material->GetProperties();
                    // PBR材质参数
                    XMFLOAT3 albedo = { props.baseColor.x, props.baseColor.y, props.baseColor.z };
                    context->SetConstantBuffer("MaterialAlbedo", reinterpret_cast<const float*>(&albedo), sizeof(XMFLOAT3));
                    context->SetConstantBuffer("MaterialMetallic", reinterpret_cast<const float*>(&props.metallic), sizeof(float));
                    context->SetConstantBuffer("MaterialRoughness", reinterpret_cast<const float*>(&props.roughness), sizeof(float));
                    context->SetConstantBuffer("MaterialEmissive", reinterpret_cast<const float*>(&props.emissive), sizeof(float));

                    // 绑定纹理 - 这里简化处理，暂时跳过纹理绑定
                    // TODO: 实现纹理资源的正确绑定
                }

                // 渲染所有子网格
                for (const auto& subMesh : mesh->subMeshes) {
                    // 设置顶点和索引缓冲区
                    if (!subMesh.vertices.empty()) {
                        const uint32_t vertexSizeInBytes = subMesh.verticesCount() * Vertex::GetVertexStride();
                        context->SetVertexBuffer(subMesh.vertices.data(), vertexSizeInBytes, Vertex::GetVertexStride());
                    }

                    if (!subMesh.indices.empty()) {
                        context->SetIndexBuffer(subMesh.indices.data(), subMesh.indicesCount(), true);
                    }

                    // 执行绘制
                    if (subMesh.indicesCount() > 0) {
                        context->DrawIndexed(subMesh.indicesCount());
                        m_stats.triangles += subMesh.indicesCount() / 3;
                    } else if (subMesh.verticesCount() > 0) {
                        context->Draw(subMesh.verticesCount());
                        m_stats.triangles += subMesh.verticesCount() / 3;
                    }
                    m_stats.drawCalls++;
                }
            } else {
                // 对于纯RenderComponent，直接调用其Render方法
                renderComponent->Render(context);
                m_stats.drawCalls++;

                // 统计三角形数量
                uint32_t indexCount = renderComponent->GetIndexCount();
                uint32_t vertexCount = renderComponent->GetVertexCount();
                if (indexCount > 0) {
                    m_stats.triangles += indexCount / 3;
                } else if (vertexCount > 0) {
                    m_stats.triangles += vertexCount / 3;
                }
            }
        }

        LOG_DEBUG("OpaquePass", "渲染完成: {0}个对象, {1}次DrawCall, {2}个三角形",
                  m_stats.objects, m_stats.drawCalls, m_stats.triangles);
    }
    catch (const std::exception& e) {
        LOG_ERROR("OpaquePass", "Exception during execution: {0}", e.what());
    }
}

void OpaquePass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
    LOG_DEBUG("OpaquePass", "设置渲染目标: 0x{0:x}", reinterpret_cast<uintptr_t>(renderTarget));
}

void OpaquePass::ClearRenderTarget(float r, float g, float b, float a)
{
    // 清屏操作通常在主渲染管线中处理
    LOG_DEBUG("OpaquePass", "清除颜色缓冲: ({0}, {1}, {2}, {3})", r, g, b, a);
}

void OpaquePass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    LOG_DEBUG("OpaquePass", "设置视口: {0}x{1}", width, height);
}

void OpaquePass::SetDepthBuffer(void* depthBuffer)
{
    m_depthBuffer = depthBuffer;
    LOG_DEBUG("OpaquePass", "设置深度缓冲区: 0x{0:x}", reinterpret_cast<uintptr_t>(depthBuffer));
}

void OpaquePass::SetViewMatrix(const XMMATRIX& view)
{
    m_view = view;
}

void OpaquePass::SetProjectionMatrix(const XMMATRIX& projection)
{
    m_projection = projection;
}

void OpaquePass::SetLights(const std::vector<Light>& lights)
{
    m_lights = lights;
    LOG_DEBUG("OpaquePass", "设置 {0} 个光源", lights.size());
}

void OpaquePass::SetAmbientLight(const XMFLOAT3& color)
{
    m_ambientLight = color;
    LOG_DEBUG("OpaquePass", "设置环境光: ({0}, {1}, {2})", color.x, color.y, color.z);
}

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine