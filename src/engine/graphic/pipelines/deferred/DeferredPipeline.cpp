#include "DeferredPipeline.h"
#include "Camera.h"
#include "Logger.h"
#include <DirectXColors.h>
#include <chrono>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

DeferredPipeline::DeferredPipeline()
    : m_renderPipe(nullptr)
    , m_camera(nullptr)
{
    LOG_DEBUG("DeferredPipeline", "延迟渲染管线构造函数被调用");
    m_stats = {};
}

DeferredPipeline::~DeferredPipeline()
{
    LOG_DEBUG("DeferredPipeline", "延迟渲染管线析构函数被调用");
    Shutdown();
}

bool DeferredPipeline::Initialize(ScriptableRenderPipeline* renderPipe)
{
    LOG_INFO("DeferredPipeline", "初始化延迟渲染管线");

    if (!renderPipe) {
        LOG_ERROR("DeferredPipeline", "Invalid render pipe provided");
        return false;
    }

    m_renderPipe = renderPipe;

    // 初始化各种渲染通道
    if (!InitializeRenderPasses()) {
        LOG_ERROR("DeferredPipeline", "Failed to initialize render passes");
        return false;
    }

    // 创建渲染目标
    if (!CreateRenderTargets()) {
        LOG_ERROR("DeferredPipeline", "Failed to create render targets");
        return false;
    }

    // 设置默认环境光
    SetAmbientLight(DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f));

    // 添加默认方向光
    Light defaultLight;
    defaultLight.type = LightType::Directional;
    defaultLight.direction = DirectX::XMFLOAT3(0, -1.0f, -1.0f);
    DirectX::XMStoreFloat3(&defaultLight.direction,
        DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&defaultLight.direction)));
    defaultLight.color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
    defaultLight.intensity = 1.0f;
    defaultLight.castShadows = true;
    AddLight(defaultLight);

    // 启用默认后处理效果
    SetPostProcessEffect(PostProcessEffect::ToneMapping, true);
    SetPostProcessEffect(PostProcessEffect::GammaCorrection, true);

    LOG_INFO("DeferredPipeline", "延迟渲染管线初始化成功");
    return true;
}

void DeferredPipeline::Shutdown()
{
    LOG_INFO("DeferredPipeline", "关闭延迟渲染管线");
    Cleanup();
}

void DeferredPipeline::Update(float deltaTime)
{
    if (!m_renderPipe || !m_camera) {
        return;
    }

    // 更新渲染统计
    m_stats.lastFrameTime = deltaTime;

    // 获取相机矩阵
    DirectX::XMMATRIX view = m_camera->GetViewMatrix();
    DirectX::XMMATRIX projection = m_camera->GetProjectionMatrix();
    DirectX::XMMATRIX viewProjection = view * projection;

    // 更新几何通道
    if (m_geometryPass) {
        m_geometryPass->SetViewMatrix(view);
        m_geometryPass->SetProjectionMatrix(projection);
    }

    // 更新天空盒通道
    if (m_skyboxRenderPass) {
        // 天空盒需要特殊的视图矩阵（移除平移）
        DirectX::XMMATRIX skyboxView = view;
        skyboxView.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        m_skyboxRenderPass->SetViewProjectionMatrix(skyboxView * projection);
    }

    // 更新光照通道
    if (m_lightingPass) {
        m_lightingPass->SetLights(m_lights);
        m_lightingPass->SetAmbientLight(m_ambientLight);
    }

    // 更新透明物体通道
    if (m_transparentPass) {
        m_transparentPass->SetViewMatrix(view);
        m_transparentPass->SetProjectionMatrix(projection);
    }
}

void DeferredPipeline::SetCamera(ICamera* camera)
{
    m_camera = camera;
    LOG_DEBUG("DeferredPipeline", "设置相机: {0}", camera ? "有效" : "无效");
}

void DeferredPipeline::AddLight(const Light& light)
{
    m_lights.push_back(light);
    LOG_DEBUG("DeferredPipeline", "添加光源，当前总数: {0}", m_lights.size());
}

void DeferredPipeline::ClearLights()
{
    m_lights.clear();
    LOG_DEBUG("DeferredPipeline", "清除所有光源");
}

void DeferredPipeline::SetLights(const std::vector<Light>& lights)
{
    m_lights = lights;
    LOG_DEBUG("DeferredPipeline", "设置光源数量: {0}", lights.size());
}

void DeferredPipeline::SetAmbientLight(const DirectX::XMFLOAT3& ambient)
{
    m_ambientLight = ambient;
    LOG_DEBUG("DeferredPipeline", "设置环境光: ({0}, {1}, {2})", ambient.x, ambient.y, ambient.z);
}

void DeferredPipeline::SetPostProcessEffect(PostProcessEffect effect, bool enable)
{
    if (m_compositionPass) {
        m_compositionPass->SetPostProcessEffect(effect, enable);
    }
}

bool DeferredPipeline::IsPostProcessEffectEnabled(PostProcessEffect effect) const
{
    if (m_compositionPass) {
        return m_compositionPass->IsPostProcessEffectEnabled(effect);
    }
    return false;
}

bool DeferredPipeline::InitializeRenderPasses()
{
    LOG_DEBUG("DeferredPipeline", "初始化渲染通道");

    // 1. 创建G-Buffer
    m_gbuffer = std::make_shared<GBuffer>();
    if (!m_gbuffer->Create(1920, 1080)) {
        LOG_ERROR("DeferredPipeline", "Failed to create G-Buffer");
        return false;
    }

    // 2. 几何通道
    m_geometryPass = std::make_shared<GeometryPass>();
    m_geometryPass->SetGBuffer(m_gbuffer);
    m_geometryPass->SetViewport(1920, 1080);
    m_renderPipe->AddRenderPass(m_geometryPass);

    // 3. 天空盒通道
    m_skyboxRenderPass = std::make_shared<SkyboxRenderPass>();
    m_skyboxRenderPass->SetViewport(1920, 1080);
    m_renderPipe->AddRenderPass(m_skyboxRenderPass);

    // 4. 光照通道
    m_lightingPass = std::make_shared<LightingPass>();
    m_lightingPass->SetGBuffer(m_gbuffer);
    m_lightingPass->SetViewport(1920, 1080);
    m_renderPipe->AddRenderPass(m_lightingPass);

    // 5. 透明物体通道（前向渲染）
    m_transparentPass = std::make_shared<TransparentPass>();
    m_transparentPass->SetViewport(1920, 1080);
    m_transparentPass->SetDepthWrite(false);
    m_renderPipe->AddRenderPass(m_transparentPass);

    // 6. 合成通道
    m_compositionPass = std::make_shared<CompositionPass>();
    m_compositionPass->SetViewport(1920, 1080);
    m_renderPipe->AddRenderPass(m_compositionPass);

    return true;
}

bool DeferredPipeline::CreateRenderTargets()
{
    LOG_DEBUG("DeferredPipeline", "创建渲染目标");

    // TODO: 创建以下渲染目标
    // 1. 光照缓冲区
    // 2. AO缓冲区（如果启用SSAO）
    // 3. 泛光缓冲区（如果启用Bloom）

    return true;
}

void DeferredPipeline::Cleanup()
{
    LOG_DEBUG("DeferredPipeline", "清理资源");

    m_geometryPass.reset();
    m_skyboxRenderPass.reset();
    m_lightingPass.reset();
    m_transparentPass.reset();
    m_compositionPass.reset();
    m_gbuffer.reset();

    m_renderPipe = nullptr;
    m_camera = nullptr;

    LOG_INFO("DeferredPipeline", "延迟渲染管线清理完成");
}

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine