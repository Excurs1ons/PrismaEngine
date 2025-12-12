#include "ForwardPipeline.h"
#include "graphic/pipelines/SkyboxRenderPass.h"
#include "graphic/pipelines/forward/DepthPrePass.h"
#include "graphic/pipelines/forward/OpaquePass.h"
#include "graphic/pipelines/forward/TransparentPass.h"
#include "Camera.h"
#include "Logger.h"
#include <DirectXColors.h>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

ForwardPipeline::ForwardPipeline()
    : m_renderPipe(nullptr)
    , m_camera(nullptr)
{
    LOG_DEBUG("ForwardPipeline", "构造函数被调用");
    m_stats = {};
}

ForwardPipeline::~ForwardPipeline()
{
    LOG_DEBUG("ForwardPipeline", "析构函数被调用");
    Shutdown();
}

bool ForwardPipeline::Initialize(ScriptableRenderPipeline* renderPipe)
{
    LOG_DEBUG("ForwardPipeline", "初始化前向渲染管线");

    if (!renderPipe) {
        LOG_ERROR("ForwardPipeline", "Invalid render pipe provided");
        return false;
    }

    m_renderPipe = renderPipe;

    // 创建所有渲染通道
    LOG_DEBUG("ForwardPipeline", "创建渲染通道");

    // 1. 深度预渲染通道（优化遮挡剔除）
    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_depthPrePass->SetViewport(1920, 1080); // 默认尺寸，会在Resize时更新
    m_renderPipe->AddRenderPass(m_depthPrePass);

    // 2. 天空盒渲染通道
    m_skyboxRenderPass = std::make_shared<SkyboxRenderPass>();
    m_skyboxRenderPass->SetViewport(1920, 1080);
    m_renderPipe->AddRenderPass(m_skyboxRenderPass);

    // 3. 不透明物体渲染通道（主要渲染）
    m_opaquePass = std::make_shared<OpaquePass>();
    m_opaquePass->SetViewport(1920, 1080);
    // 设置默认环境光
    m_opaquePass->SetAmbientLight(XMFLOAT3(0.1f, 0.1f, 0.1f));
    // 设置默认方向光
    OpaquePass::Light directionalLight;
    directionalLight.type = 0; // 方向光
    directionalLight.position = XMFLOAT3(0, 100, 0);
    directionalLight.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    directionalLight.intensity = 1.0f;
    directionalLight.direction = XMFLOAT3(0, -1.0f, -1.0f);
    XMStoreFloat3(&directionalLight.direction, XMVector3Normalize(XMLoadFloat3(&directionalLight.direction)));
    m_opaquePass->SetLights({directionalLight});
    m_renderPipe->AddRenderPass(m_opaquePass);

    // 4. 透明物体渲染通道
    m_transparentPass = std::make_shared<TransparentPass>();
    m_transparentPass->SetViewport(1920, 1080);
    m_transparentPass->SetDepthWrite(false); // 透明物体不写入深度
    m_renderPipe->AddRenderPass(m_transparentPass);

    LOG_INFO("ForwardPipeline", "Forward rendering pipeline initialized successfully with {0} passes", 4);
    return true;
}

void ForwardPipeline::Shutdown()
{
    LOG_DEBUG("ForwardPipeline", "关闭前向渲染管线");

    m_depthPrePass.reset();
    m_skyboxRenderPass.reset();
    m_opaquePass.reset();
    m_transparentPass.reset();

    m_renderPipe = nullptr;
    m_camera = nullptr;

    LOG_INFO("ForwardPipeline", "Forward rendering pipeline shutdown completed");
}

void ForwardPipeline::Update(float deltaTime)
{
    if (!m_renderPipe || !m_camera) {
        return;
    }

    // 更新渲染统计
    m_stats.lastFrameTime = deltaTime;

    // 获取相机矩阵
    XMMATRIX view = m_camera->GetViewMatrix();
    XMMATRIX projection = m_camera->GetProjectionMatrix();
    XMMATRIX viewProjection = view * projection;

    // 更新所有Pass的相机矩阵
    if (m_depthPrePass) {
        m_depthPrePass->SetViewProjectionMatrix(viewProjection);
    }

    if (m_skyboxRenderPass) {
        // 天空盒需要特殊的视图矩阵（移除平移）
        XMMATRIX skyboxView = view;
        skyboxView.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        m_skyboxRenderPass->SetViewProjectionMatrix(skyboxView * projection);
    }

    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(projection);
    }

    if (m_transparentPass) {
        m_transparentPass->SetViewMatrix(view);
        m_transparentPass->SetProjectionMatrix(projection);
    }
}

void ForwardPipeline::SetCamera(ICamera* camera)
{
    m_camera = camera;
    LOG_DEBUG("ForwardPipeline", "设置相机: {0}", camera ? "有效" : "无效");
}

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine