#include "ForwardPipeline.h"
#include "pipelines/forward/DepthPrePass.h"
#include "pipelines/forward/OpaquePass.h"
#include "pipelines/SkyboxRenderPass.h"
#include "pipelines/forward/TransparentPass.h"
#include "ui/UIPass.h"

namespace PrismaEngine::Graphic {

ForwardPipeline::ForwardPipeline()
    : LogicalForwardPipeline()
    , m_camera(nullptr) {
    m_stats = {};
}

ForwardPipeline::~ForwardPipeline() {
    // Pass 会通过 shared_ptr 自动释放
}

bool ForwardPipeline::Initialize() {
    // 创建所有 Pass
    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_opaquePass = std::make_shared<OpaquePass>();
    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_transparentPass = std::make_shared<TransparentPass>();
    m_uiPass = std::make_shared<PrismaEngine::UIPass>();

    // 添加到 Pipeline（会按优先级自动排序）
    AddPass(m_depthPrePass.get());
    AddPass(m_opaquePass.get());
    AddPass(m_skyboxPass.get());
    AddPass(m_transparentPass.get());
    AddPass(m_uiPass.get());

    // 设置默认环境光
    m_opaquePass->SetAmbientColor(PrismaMath::vec3(0.1f, 0.1f, 0.1f));
    m_opaquePass->SetAmbientIntensity(1.0f);

    // 设置默认方向光
    Light directionalLight;
    directionalLight.position = PrismaMath::vec3(0.0f, 100.0f, 0.0f);
    directionalLight.color = PrismaMath::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    directionalLight.direction = PrismaMath::vec3(0.0f, -1.0f, -1.0f);
    directionalLight.type = 0; // 方向光
    m_opaquePass->SetLights({directionalLight});

    // 透明物体不写入深度
    m_transparentPass->SetDepthWrite(false);
    m_transparentPass->SetDepthTest(true);

    // 启用自动排序
    SetAutoSort(true);

    return true;
}

void ForwardPipeline::Update(float deltaTime, Engine::Graphic::ICamera* camera) {
    m_camera = camera;
    m_stats.lastFrameTime = deltaTime;

    // 更新所有 Pass 的时间
    if (m_depthPrePass) m_depthPrePass->Update(deltaTime);
    if (m_opaquePass) m_opaquePass->Update(deltaTime);
    if (m_skyboxPass) m_skyboxPass->Update(deltaTime);
    if (m_transparentPass) m_transparentPass->Update(deltaTime);
    if (m_uiPass) m_uiPass->Update(deltaTime);

    // 更新相机数据
    if (m_camera) {
        UpdatePassesCameraData(m_camera);
    }
}

void ForwardPipeline::Execute(const PassExecutionContext& context) {
    // 先执行基类的 Execute（按优先级执行所有 Pass）
    LogicalForwardPipeline::Execute(context);

    // 收集渲染统计
    CollectStats();
}

void ForwardPipeline::UpdatePassesCameraData(Engine::Graphic::ICamera* camera) {
    if (!camera) {
        return;
    }

    // 从相机接口获取视图和投影矩阵
    PrismaMath::mat4 view = camera->GetViewMatrix();
    PrismaMath::mat4 projection = camera->GetProjectionMatrix();

    // 更新所有需要相机矩阵的 Pass
    if (m_depthPrePass) {
        m_depthPrePass->SetViewMatrix(view);
        m_depthPrePass->SetProjectionMatrix(projection);
    }

    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(projection);
    }

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

void ForwardPipeline::CollectStats() {
    m_stats.totalDrawCalls = 0;
    m_stats.totalTriangles = 0;

    if (m_depthPrePass) {
        const auto& stats = m_depthPrePass->GetRenderStats();
        m_stats.totalDrawCalls += stats.drawCalls;
        m_stats.totalTriangles += stats.triangles;
    }

    if (m_opaquePass) {
        const auto& stats = m_opaquePass->GetRenderStats();
        m_stats.totalDrawCalls += stats.drawCalls;
        m_stats.totalTriangles += stats.triangles;
        m_stats.opaqueObjects = stats.objects;
    }

    if (m_transparentPass) {
        const auto& stats = m_transparentPass->GetRenderStats();
        m_stats.totalDrawCalls += stats.drawCalls;
        m_stats.totalTriangles += stats.triangles;
        m_stats.transparentObjects = stats.transparentObjects;
    }
}

} // namespace PrismaEngine::Graphic
