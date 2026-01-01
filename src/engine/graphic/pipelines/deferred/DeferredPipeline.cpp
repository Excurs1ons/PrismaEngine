#include "DeferredPipeline.h"
#include "pipelines/deferred/GeometryPass.h"
#include "pipelines/deferred/LightingPass.h"
#include "pipelines/deferred/CompositionPass.h"
#include "pipelines/SkyboxRenderPass.h"
#include "pipelines/forward/TransparentPass.h"
#include "graphic/ICamera.h"

namespace PrismaEngine::Graphic {

DeferredPipeline::DeferredPipeline()
    : LogicalDeferredPipeline()
    , m_camera(nullptr)
    , m_ambientLight(0.1f, 0.1f, 0.1f) {
    m_stats = {};
}

DeferredPipeline::~DeferredPipeline() {
    // Pass 会通过 shared_ptr 自动释放
}

bool DeferredPipeline::Initialize() {
    // TODO: 创建所有 Pass
    // 这些 Pass 类需要先被重构为逻辑 Pass
    //
    // m_geometryPass = std::make_shared<GeometryPass>();
    // m_skyboxPass = std::make_shared<SkyboxPass>();
    // m_lightingPass = std::make_shared<LightingPass>();
    // m_transparentPass = std::make_shared<TransparentPass>();
    // m_compositionPass = std::make_shared<CompositionPass>();
    //
    // 添加到 Pipeline
    // AddPass(m_geometryPass.get());
    // AddPass(m_skyboxPass.get());
    // AddPass(m_lightingPass.get());
    // AddPass(m_transparentPass.get());
    // AddPass(m_compositionPass.get());

    // 设置默认环境光
    SetAmbientLight(PrismaMath::vec3(0.1f, 0.1f, 0.1f));

    // 添加默认方向光
    Light defaultLight;
    defaultLight.type = LightType::Directional;
    defaultLight.direction = PrismaMath::vec3(0.0f, -1.0f, -1.0f);
    // Normalize direction
    float length = sqrtf(defaultLight.direction.x * defaultLight.direction.x +
                         defaultLight.direction.y * defaultLight.direction.y +
                         defaultLight.direction.z * defaultLight.direction.z);
    if (length > 0.0f) {
        defaultLight.direction = defaultLight.direction / length;
    }
    defaultLight.color = PrismaMath::vec3(1.0f, 1.0f, 1.0f);
    defaultLight.intensity = 1.0f;
    defaultLight.castShadows = true;
    AddLight(defaultLight);

    // 启用默认后处理效果
    SetPostProcessEffect(PostProcessEffect::ToneMapping, true);
    SetPostProcessEffect(PostProcessEffect::GammaCorrection, true);

    // 启用自动排序
    SetAutoSort(true);

    return true;
}

void DeferredPipeline::Update(float deltaTime, PrismaEngine::Graphic::ICamera* camera) {
    m_camera = camera;
    m_stats.lastFrameTime = deltaTime;

    // 更新所有 Pass 的时间
    if (m_geometryPass) m_geometryPass->Update(deltaTime);
    if (m_skyboxPass) m_skyboxPass->Update(deltaTime);
    if (m_lightingPass) m_lightingPass->Update(deltaTime);
    if (m_transparentPass) m_transparentPass->Update(deltaTime);
    if (m_compositionPass) m_compositionPass->Update(deltaTime);

    // 更新相机数据
    if (m_camera) {
        UpdatePassesCameraData(m_camera);
    }

    // 更新光照 Pass 的光源数据
    if (m_lightingPass) {
        // TODO: m_lightingPass->SetLights(m_lights);
        // TODO: m_lightingPass->SetAmbientLight(m_ambientLight);
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
        // TODO: m_compositionPass->SetPostProcessEffect(effect, enable);
    }
}

bool DeferredPipeline::IsPostProcessEffectEnabled(PostProcessEffect effect) const {
    if (m_compositionPass) {
        // TODO: return m_compositionPass->IsPostProcessEffectEnabled(effect);
    }
    return false;
}

void DeferredPipeline::UpdatePassesCameraData(PrismaEngine::Graphic::ICamera* camera) {
    if (!camera) {
        return;
    }

    // 从相机接口获取视图和投影矩阵
    PrismaMath::mat4 view = camera->GetViewMatrix();
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
        skyboxView[3] = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        m_skyboxPass->SetViewMatrix(skyboxView);
        m_skyboxPass->SetProjectionMatrix(projection);
    }
}

void DeferredPipeline::CollectStats() {
    // TODO: 从各个 Pass 收集渲染统计
    m_stats.geometryPassObjects = 0;
    m_stats.geometryPassTriangles = 0;
    m_stats.lightingPassLights = static_cast<uint32_t>(m_lights.size());
    m_stats.transparentObjects = 0;
}

} // namespace PrismaEngine::Graphic
