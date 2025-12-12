#include "LightingPass.h"
#include "Camera.h"
#include "SceneManager.h"
#include "Logger.h"

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

LightingPass::LightingPass()
{
    LOG_DEBUG("LightingPass", "光照通道构造函数被调用");
    m_stats = {};
}

LightingPass::~LightingPass()
{
    LOG_DEBUG("LightingPass", "光照通道析构函数被调用");
}

void LightingPass::Execute(RenderCommandContext* context)
{
    if (!context || !m_gbuffer || !m_renderTarget) {
        LOG_ERROR("LightingPass", "无效的上下文、G-Buffer或渲染目标");
        return;
    }

    LOG_DEBUG("LightingPass", "开始执行光照通道");
    m_stats = {};
    m_stats.shadowCastingLights = 0;

    try {
        // 设置渲染目标
        context->SetRenderTargets(&m_renderTarget, 1, m_gbuffer->GetDepthStencilView());

        // 设置视口
        context->SetViewport(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));

        // 清除渲染目标为环境光
        context->ClearRenderTarget(m_renderTarget,
                                   m_ambientLight.x,
                                   m_ambientLight.y,
                                   m_ambientLight.z, 1.0f);

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

        // 获取相机位置和矩阵
        DirectX::XMMATRIX view = camera->GetViewMatrix();
        DirectX::XMMATRIX projection = camera->GetProjectionMatrix();
        DirectX::XMMATRIX viewProjection = view * projection;

        DirectX::XMMATRIX cameraWorld = DirectX::XMMatrixInverse(view, nullptr);
        DirectX::XMFLOAT3 cameraPos = DirectX::XMFLOAT3(
            DirectX::XMVectorGetX(cameraWorld.r[3]),
            DirectX::XMVectorGetY(cameraWorld.r[3]),
            DirectX::XMVectorGetZ(cameraWorld.r[3])
        );

        // 设置相机参数
        context->SetConstantBuffer("CameraPosition", &cameraPos, sizeof(DirectX::XMFLOAT3));
        context->SetConstantBuffer("ViewMatrix", view);
        context->SetConstantBuffer("ProjectionMatrix", projection);
        context->SetConstantBuffer("ViewProjectionMatrix", viewProjection);

        // 设置环境光
        context->SetConstantBuffer("AmbientLight", &m_ambientLight, sizeof(DirectX::XMFLOAT3));

        // 绑定G-Buffer作为着色器资源
        m_gbuffer->SetAsShaderResources(context);

        // 绑定IBL纹理（如果启用）
        if (m_iblEnabled) {
            if (m_irradianceMap) {
                context->SetShaderResource("IrradianceMap", m_irradianceMap);
            }
            if (m_prefilterMap) {
                context->SetShaderResource("PrefilterMap", m_prefilterMap);
            }
            if (m_brdfLUT) {
                context->SetShaderResource("BRDFLUT", m_brdfLUT);
            }
        }

        // 设置混合模式（加法混合）
        context->SetBlendState(true);

        // 渲染光照
        for (const auto& light : m_lights) {
            m_stats.lightsRendered++;

            switch (light.type) {
            case LightType::Directional:
                ApplyDirectionalLight(context, light);
                if (light.castShadows) m_stats.shadowCastingLights++;
                break;
            case LightType::Point:
                ApplyPointLight(context, light);
                if (light.castShadows) m_stats.shadowCastingLights++;
                break;
            case LightType::Spot:
                ApplySpotLight(context, light);
                if (light.castShadows) m_stats.shadowCastingLights++;
                break;
            }
        }

        // 禁用混合
        context->SetBlendState(false);

        LOG_DEBUG("LightingPass", "光照通道完成 - 光源数: {0}, 投影光源: {1}",
                  m_stats.lightsRendered, m_stats.shadowCastingLights);
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

void LightingPass::RenderFullScreenQuad(RenderCommandContext* context)
{
    // TODO: 渲染全屏四边形
    // 这需要使用预先准备的顶点缓冲区或使用顶点着色器生成
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