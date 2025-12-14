#include "LightingPass.h"
#include "Camera.h"
#include "SceneManager.h"
#include "Shader.h"
#include "DefaultShader.h"
#include "Logger.h"
#include <vector>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

LightingPass::LightingPass()
{
    LOG_DEBUG("LightingPass", "光照通道构造函数被调用");
    m_stats = {};

    // 创建延迟渲染光照通道着色器
    m_shader = std::make_shared<Shader>();
    if (m_shader && m_shader->CompileFromString(Graphic::DEFERRED_LIGHTING_VERTEX_SHADER, Graphic::DEFERRED_LIGHTING_PIXEL_SHADER)) {
        m_shader->SetName("DeferredLightingPass");
        LOG_INFO("LightingPass", "延迟渲染光照通道着色器编译成功");
    } else {
        LOG_ERROR("LightingPass", "延迟渲染光照通道着色器编译失败");
        m_shader.reset();
    }

    // 创建全屏四边形几何体
    CreateFullScreenQuad();
}

LightingPass::~LightingPass()
{
    LOG_DEBUG("LightingPass", "光照通道析构函数被调用");
}

void LightingPass::Execute(RenderCommandContext* context)
{
    if (!context || !m_gbuffer || !m_renderTarget || !m_shader) {
        LOG_ERROR("LightingPass", "无效的上下文、G-Buffer、渲染目标或着色器");
        return;
    }

    LOG_DEBUG("LightingPass", "开始执行光照通道");
    m_stats = {};

    try {
        // 设置渲染目标
        context->SetRenderTargets(&m_renderTarget, 1, nullptr);

        // 设置视口
        context->SetViewport(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));

        // 清除渲染目标
        context->ClearRenderTarget(m_renderTarget, 0.0f, 0.0f, 0.0f, 1.0f);

        // 获取相机
        auto scene = SceneManager::GetInstance()->GetCurrentScene();
        if (!scene) {
            LOG_WARNING("LightingPass", "没有活动场景");
            return;
        }

        auto camera = scene->GetMainCamera();
        if (!camera) {
            LOG_WARNING("LightingPass", "没有主相机");
            return;
        }

        // 获取相机位置
        DirectX::XMMATRIX view = camera->GetViewMatrix();
        DirectX::XMMATRIX projection = camera->GetProjectionMatrix();
        DirectX::XMMATRIX viewProjection = view * projection;
        DirectX::XMMATRIX invViewProjection = DirectX::XMMatrixInverse(nullptr, viewProjection);

        DirectX::XMMATRIX cameraWorld = DirectX::XMMatrixInverse(view, nullptr);
        DirectX::XMFLOAT3 cameraPos = DirectX::XMFLOAT3(
            DirectX::XMVectorGetX(cameraWorld.r[3]),
            DirectX::XMVectorGetY(cameraWorld.r[3]),
            DirectX::XMVectorGetZ(cameraWorld.r[3])
        );

        // 绑定G-Buffer作为着色器资源
        m_gbuffer->SetAsShaderResources(context);

        // 设置着色器常量
        context->SetShader(m_shader.get());

        // 相机缓冲区
        struct CameraBuffer {
            DirectX::XMFLOAT3 cameraPosition;
            float padding1;
            DirectX::XMMATRIX inverseViewProjection;
        } cameraBuffer;
        cameraBuffer.cameraPosition = cameraPos;
        cameraBuffer.inverseViewProjection = invViewProjection;
        context->SetConstantBuffer("CameraBuffer", &cameraBuffer, sizeof(CameraBuffer));

        // 环境光缓冲区
        struct AmbientBuffer {
            DirectX::XMFLOAT3 ambientColor;
            float ambientIntensity;
        } ambientBuffer;
        ambientBuffer.ambientColor = m_ambientLight;
        ambientBuffer.ambientIntensity = 1.0f;
        context->SetConstantBuffer("AmbientBuffer", &ambientBuffer, sizeof(AmbientBuffer));

        // 渲染所有光源（简化版本 - 目前只支持单个方向光）
        for (const auto& light : m_lights) {
            m_stats.lightsRendered++;

            // 光照缓冲区
            struct LightBuffer {
                DirectX::XMFLOAT3 lightDirection;
                float lightType;
                DirectX::XMFLOAT3 lightColor;
                float lightIntensity;
                DirectX::XMFLOAT3 lightPosition;
                float lightRadius;
                DirectX::XMFLOAT3 lightAttenuation;
                float padding2;
                DirectX::XMMATRIX lightViewProjection;
            } lightBuffer;

            lightBuffer.lightDirection = light.direction;
            lightBuffer.lightType = static_cast<float>(light.type);
            lightBuffer.lightColor = light.color;
            lightBuffer.lightIntensity = light.intensity;
            lightBuffer.lightPosition = light.position;
            lightBuffer.lightRadius = light.range;
            lightBuffer.lightAttenuation = DirectX::XMFLOAT3(1.0f, 0.1f, 0.01f); // 默认衰减
            lightBuffer.lightViewProjection = light.shadowMatrix;

            context->SetConstantBuffer("LightBuffer", &lightBuffer, sizeof(LightBuffer));

            // 渲染全屏四边形
            RenderFullScreenQuad(context);

            // 如果需要渲染多个光源，使用加法混合
            if (m_lights.size() > 1) {
                context->SetBlendState(true);
            }
        }

        // 禁用混合
        context->SetBlendState(false);

        LOG_DEBUG("LightingPass", "光照通道完成 - 光源数: {0}", m_stats.lightsRendered);
    }
    catch (const std::exception& e) {
        LOG_ERROR("LightingPass", "执行异常: {0}", e.what());
    }
}

void LightingPass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
    LOG_DEBUG("LightingPass", "设置渲染目标: 0x{0:x}",
              reinterpret_cast<uintptr_t>(renderTarget));
}

void LightingPass::ClearRenderTarget(float r, float g, float b, float a)
{
    if (m_renderTarget) {
        LOG_DEBUG("LightingPass", "清除渲染目标: ({0}, {1}, {2}, {3})", r, g, b, a);
        // TODO: 实现渲染目标清除
    }
}

void LightingPass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    LOG_DEBUG("LightingPass", "设置视口: {0}x{1}", width, height);
}

void LightingPass::SetGBuffer(std::shared_ptr<GBuffer> gbuffer)
{
    m_gbuffer = gbuffer;
    LOG_DEBUG("LightingPass", "设置G-Buffer: 0x{0:x}",
              reinterpret_cast<uintptr_t>(gbuffer.get()));
}

void LightingPass::SetAmbientLight(const DirectX::XMFLOAT3& ambient)
{
    m_ambientLight = ambient;
    LOG_DEBUG("LightingPass", "设置环境光: ({0}, {1}, {2})", ambient.x, ambient.y, ambient.z);
}

void LightingPass::SetLights(const std::vector<Light>& lights)
{
    m_lights = lights;
    LOG_DEBUG("LightingPass", "设置光源数量: {0}", lights.size());
}

void LightingPass::AddLight(const Light& light)
{
    m_lights.push_back(light);
    LOG_DEBUG("LightingPass", "添加光源，当前总数: {0}", m_lights.size());
}

void LightingPass::ClearLights()
{
    m_lights.clear();
    LOG_DEBUG("LightingPass", "清除所有光源");
}

void LightingPass::SetIBL(bool enable)
{
    m_iblEnabled = enable;
    LOG_DEBUG("LightingPass", "IBL: {0}", enable ? "启用" : "禁用");
}

void LightingPass::SetIBLTextures(void* irradianceMap, void* prefilterMap, void* brdfLUT)
{
    m_irradianceMap = irradianceMap;
    m_prefilterMap = prefilterMap;
    m_brdfLUT = brdfLUT;
    LOG_DEBUG("LightingPass", "设置IBL纹理");
}

void LightingPass::CreateFullScreenQuad()
{
    // 创建全屏四边形顶点 (位置 + UV)
    m_fullScreenVertices = {
        // Position (x, y, z)    UV (u, v)
        -1.0f,  1.0f, 0.0f,    0.0f, 0.0f,  // 左上
         1.0f,  1.0f, 0.0f,    1.0f, 0.0f,  // 右上
         1.0f, -1.0f, 0.0f,    1.0f, 1.0f,  // 右下
        -1.0f, -1.0f, 0.0f,    0.0f, 1.0f   // 左下
    };

    // 创建索引
    m_fullScreenIndices = {
        0, 1, 2,  // 第一个三角形
        0, 2, 3   // 第二个三角形
    };

    LOG_DEBUG("LightingPass", "创建全屏四边形几何体");
}

void LightingPass::RenderFullScreenQuad(RenderCommandContext* context)
{
    if (!context || m_fullScreenVertices.empty() || m_fullScreenIndices.empty()) {
        LOG_ERROR("LightingPass", "无效的上下文或全屏四边形数据");
        return;
    }

    // 设置着色器
    context->SetShader(m_shader.get());

    // 设置顶点缓冲区
    context->SetVertexBuffer(
        m_fullScreenVertices.data(),
        static_cast<uint32_t>(m_fullScreenVertices.size() * sizeof(float)),
        5 * sizeof(float) // 3个位置 + 2个UV
    );

    // 设置索引缓冲区
    context->SetIndexBuffer(
        m_fullScreenIndices.data(),
        static_cast<uint32_t>(m_fullScreenIndices.size() * sizeof(uint16_t)),
        true
    );

    // 绘制
    context->DrawIndexed(static_cast<uint32_t>(m_fullScreenIndices.size()));
}

void LightingPass::ApplyDirectionalLight(RenderCommandContext* context, const Light& light)
{
    LOG_DEBUG("LightingPass", "应用方向光: 颜色=({0}, {1}, {2}), 强度={3}",
              light.color.x, light.color.y, light.color.z, light.intensity);

    // 设置光源参数
    context->SetConstantBuffer("LightType", &light.type, sizeof(uint32_t));
    context->SetConstantBuffer("LightDirection", &light.direction, sizeof(DirectX::XMFLOAT3));
    context->SetConstantBuffer("LightColor", &light.color, sizeof(DirectX::XMFLOAT3));
    context->SetConstantBuffer("LightIntensity", &light.intensity, sizeof(float));
    context->SetConstantBuffer("LightCastShadows", &light.castShadows, sizeof(bool));

    if (light.castShadows) {
        context->SetConstantBuffer("ShadowMatrix", &light.shadowMatrix, sizeof(DirectX::XMMATRIX));
        // TODO: 绑定阴影贴图
        context->SetShaderResource("ShadowMap", nullptr);
    }

    // 渲染全屏
    RenderFullScreenQuad(context);
}

void LightingPass::ApplyPointLight(RenderCommandContext* context, const Light& light)
{
    LOG_DEBUG("LightingPass", "应用点光源: 位置=({0}, {1}, {2}), 范围={3}",
              light.position.x, light.position.y, light.position.z, light.range);

    // 设置光源参数
    context->SetConstantBuffer("LightType", &light.type, sizeof(uint32_t));
    context->SetConstantBuffer("LightPosition", &light.position, sizeof(DirectX::XMFLOAT3));
    context->SetConstantBuffer("LightColor", &light.color, sizeof(DirectX::XMFLOAT3));
    context->SetConstantBuffer("LightIntensity", &light.intensity, sizeof(float));
    context->SetConstantBuffer("LightRange", &light.range, sizeof(float));
    context->SetConstantBuffer("LightCastShadows", &light.castShadows, sizeof(bool));

    // TODO: 使用光球体几何体而不是全屏四边形
    RenderFullScreenQuad(context);
}

void LightingPass::ApplySpotLight(RenderCommandContext* context, const Light& light)
{
    LOG_DEBUG("LightingPass", "应用聚光灯: 位置=({0}, {1}, {2}), 方向=({3}, {4}, {5})",
              light.position.x, light.position.y, light.position.z,
              light.direction.x, light.direction.y, light.direction.z);

    // 设置光源参数
    context->SetConstantBuffer("LightType", &light.type, sizeof(uint32_t));
    context->SetConstantBuffer("LightPosition", &light.position, sizeof(DirectX::XMFLOAT3));
    context->SetConstantBuffer("LightDirection", &light.direction, sizeof(DirectX::XMFLOAT3));
    context->SetConstantBuffer("LightColor", &light.color, sizeof(DirectX::XMFLOAT3));
    context->SetConstantBuffer("LightIntensity", &light.intensity, sizeof(float));
    context->SetConstantBuffer("LightRange", &light.range, sizeof(float));
    context->SetConstantBuffer("LightInnerCone", &light.innerCone, sizeof(float));
    context->SetConstantBuffer("LightOuterCone", &light.outerCone, sizeof(float));
    context->SetConstantBuffer("LightCastShadows", &light.castShadows, sizeof(bool));

    // TODO: 使用圆锥体几何体
    RenderFullScreenQuad(context);
}

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine