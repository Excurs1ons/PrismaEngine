#include "DeferredPipeline.h"
#include "GBuffer.h"
#include "graphic/ICamera.h"
#include "../SkyboxRenderPass.h"
#include "CompositionPass.h"
#include "GeometryPass.h"
#include "LightingPass.h"
#include "../forward/TransparentPass.h"

namespace Prisma::Graphic {

DeferredPipeline::DeferredPipeline() : LogicalDeferredPipeline() {
    m_camera = nullptr;
    m_ambientLight = PrismaMath::vec3(0.1f, 0.1f, 0.1f);
    m_stats = {};
}

DeferredPipeline::~DeferredPipeline() {
}

bool DeferredPipeline::Initialize() {
    m_gBufferOwner = std::make_shared<GBuffer>();
    if (!m_gBufferOwner->Initialize(m_width, m_height)) {
        return false;
    }
    m_gBuffer = m_gBufferOwner.get();

    m_geometryPass = std::make_shared<GeometryPass>();
    m_geometryPass->SetGBuffer(m_gBuffer);
    m_geometryPass->SetEnabled(true);
    AddPass(m_geometryPass.get());

    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_skyboxPass->SetEnabled(true);
    AddPass(m_skyboxPass.get());

    m_lightingPass = std::make_shared<LightingPass>();
    m_lightingPass->SetGBuffer(m_gBuffer);
    m_lightingPass->SetEnabled(true);
    AddPass(m_lightingPass.get());

    m_transparentPass = std::make_shared<TransparentPass>();
    m_transparentPass->SetEnabled(true);
    AddPass(m_transparentPass.get());

    m_compositionPass = std::make_shared<CompositionPass>();
    m_compositionPass->SetEnabled(true);
    AddPass(m_compositionPass.get());

    return true;
}

void DeferredPipeline::SetResolution(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    if (m_gBufferOwner) {
        m_gBufferOwner->Resize(width, height);
    }
}

void DeferredPipeline::Update(Prisma::Timestep ts, Prisma::Graphic::ICamera* camera) {
    m_camera              = camera;
    m_stats.lastFrameTime = ts;

    // 更新所有 Pass 的时间
    if (m_geometryPass)
        m_geometryPass->Update(ts);
    if (m_skyboxPass)
        m_skyboxPass->Update(ts);
    if (m_lightingPass)
        m_lightingPass->Update(ts);
    if (m_transparentPass)
        m_transparentPass->Update(ts);
    if (m_compositionPass)
        m_compositionPass->Update(ts);

    // 更新相机数据
    if (m_camera) {
        UpdatePassesCameraData(m_camera);
    }

    // 更新光照 Pass 的光源数据
    if (m_lightingPass) {
        // Convert DeferredPipeline::Light to LightingPass::Light
        std::vector<LightingPass::Light> lights;
        lights.reserve(m_lights.size());
        for (const auto& light : m_lights) {
            LightingPass::Light convertedLight;
            convertedLight.type = static_cast<LightingPass::LightType>(light.type);
            convertedLight.position = light.position;
            convertedLight.direction = light.direction;
            convertedLight.color = light.color;
            convertedLight.intensity = light.intensity;
            convertedLight.range = light.range;
            convertedLight.outerCone = light.spotAngle;
            convertedLight.castShadows = light.castShadows;
            lights.push_back(convertedLight);
        }
        m_lightingPass->SetLights(lights);
        m_lightingPass->SetAmbientLight(m_ambientLight);
    }
}

void DeferredPipeline::Execute(const PassExecutionContext& context) {
    // 先执行基类的 Execute（按优先级执行所有 Pass）
    LogicalDeferredPipeline::Execute(context);

    // 收集渲染统计
    CollectStats();
}

void DeferredPipeline::AddLight(const Light& light) {
    m_lights.push_back(light);
}

void DeferredPipeline::ClearLights() {
    m_lights.clear();
}

void DeferredPipeline::SetLights(const std::vector<Light>& lights) {
    m_lights = lights;
}

void DeferredPipeline::SetAmbientLight(const PrismaMath::vec3& ambient) {
    m_ambientLight = ambient;
}

void DeferredPipeline::SetPostProcessEffect(PostProcessEffect effect, bool enable) {
    if (m_compositionPass) {
        m_compositionPass->SetPostProcessEffect(
            static_cast<CompositionPass::PostProcessEffect>(effect), enable);
    }
}

bool DeferredPipeline::IsPostProcessEffectEnabled(PostProcessEffect effect) const {
    if (m_compositionPass) {
        return m_compositionPass->IsPostProcessEffectEnabled(
            static_cast<CompositionPass::PostProcessEffect>(effect));
    }
    return false;
}

void DeferredPipeline::UpdatePassesCameraData(Prisma::Graphic::ICamera* camera) {
    if (!camera) {
        return;
    }

    // 从相机接口获取视图和投影矩阵
    PrismaMath::mat4 view       = camera->GetViewMatrix();
    PrismaMath::mat4 projection = camera->GetProjectionMatrix();

    // 更新几何通道
    if (m_geometryPass) {
        m_geometryPass->SetViewMatrix(view);
        m_geometryPass->SetProjectionMatrix(projection);
    }

    // 更新透明物体通道
    if (m_transparentPass) {
        m_transparentPass->SetViewMatrix(view);
        m_transparentPass->SetProjectionMatrix(projection);
    }

    // 天空盒需要特殊的视图矩阵（移除平移部分）
    if (m_skyboxPass) {
        PrismaMath::mat4 skyboxView = view;
        skyboxView[3]               = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        m_skyboxPass->SetViewMatrix(skyboxView);
        m_skyboxPass->SetProjectionMatrix(projection);
    }
}

void DeferredPipeline::CollectStats() {
    if (m_geometryPass) {
        const auto& stats = m_geometryPass->GetRenderStats();
        m_stats.geometryPassObjects   = stats.objects;
        m_stats.geometryPassTriangles = stats.triangles;
    }
    if (m_lightingPass) {
        const auto& stats = m_lightingPass->GetRenderStats();
        m_stats.lightingPassLights = stats.lightsRendered;
    }
    if (m_transparentPass) {
        const auto& stats = m_transparentPass->GetRenderStats();
        m_stats.transparentObjects = stats.transparentObjects;
    }
    m_stats.lightingPassLights = static_cast<uint32_t>(m_lights.size());
}

}  // namespace Prisma::Graphic
