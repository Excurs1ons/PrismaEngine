#include "ForwardPipeline.h"
#include "graphic/pipelines/SkyboxRenderPass.h"
#include "graphic/pipelines/forward/DepthPrePass.h"
#include "graphic/pipelines/forward/OpaquePass.h"
#include "graphic/pipelines/forward/TransparentPass.h"
#include "Camera.h"
#include "Logger.h"
#include "SceneManager.h"
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
    if (!m_renderPipe) {
        return;
    }

    // 从SceneManager获取当前场景和相机
    auto sceneManager = SceneManager::GetInstance();
    if (sceneManager) {
        auto scene = sceneManager->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) {
                // 转换为ICamera接口
                m_camera = static_cast<Engine::Graphic::ICamera*>(camera);

                // 更新各个RenderPass的相机数据
                if (m_depthPrePass && m_camera) {
                    m_depthPrePass->SetViewMatrix(m_camera->GetViewMatrix());
                    m_depthPrePass->SetProjectionMatrix(m_camera->GetProjectionMatrix());
                }

                if (m_skyboxRenderPass && m_camera) {
                    m_skyboxRenderPass->SetViewMatrix(m_camera->GetViewMatrix());
                    m_skyboxRenderPass->SetProjectionMatrix(m_camera->GetProjectionMatrix());
                }

                if (m_opaquePass && m_camera) {
                    m_opaquePass->SetViewMatrix(m_camera->GetViewMatrix());
                    m_opaquePass->SetProjectionMatrix(m_camera->GetProjectionMatrix());
                }

                if (m_transparentPass && m_camera) {
                    m_transparentPass->SetViewMatrix(m_camera->GetViewMatrix());
                    m_transparentPass->SetProjectionMatrix(m_camera->GetProjectionMatrix());
                }
            }
        }
    }

    // 更新渲染统计
    m_stats.lastFrameTime = deltaTime;

    // 如果有相机，更新天空盒的特殊视图矩阵（移除平移）
    if (m_camera && m_skyboxRenderPass) {
        XMMATRIX view = m_camera->GetViewMatrix();
        XMMATRIX skyboxView = view;
        skyboxView.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        // 天空盒需要单独处理视图矩阵
        m_skyboxRenderPass->SetViewMatrix(skyboxView);
        m_skyboxRenderPass->SetProjectionMatrix(m_camera->GetProjectionMatrix());
    }
}

void ForwardPipeline::SetCamera(ICamera* camera)
{
    m_camera = camera;
    LOG_DEBUG("ForwardPipeline", "设置相机: {0}", camera ? "有效" : "无效");
}

void ForwardPipeline::SetRenderTargets(void* renderTarget, void* depthBuffer, uint32_t width, uint32_t height)
{
    // 设置所有RenderPass的渲染目标和深度缓冲
    if (m_depthPrePass) {
        m_depthPrePass->SetRenderTarget(renderTarget);
        m_depthPrePass->SetDepthBuffer(depthBuffer);
        m_depthPrePass->SetViewport(width, height);
    }

    if (m_skyboxRenderPass) {
        m_skyboxRenderPass->SetRenderTarget(renderTarget);
        m_skyboxRenderPass->SetViewport(width, height);
    }

    if (m_opaquePass) {
        m_opaquePass->SetRenderTarget(renderTarget);
        m_opaquePass->SetDepthBuffer(depthBuffer);
        m_opaquePass->SetViewport(width, height);
    }

    if (m_transparentPass) {
        m_transparentPass->SetRenderTarget(renderTarget);
        m_transparentPass->SetDepthBuffer(depthBuffer);
        m_transparentPass->SetViewport(width, height);
    }

    LOG_INFO("ForwardPipeline", "渲染目标和深度缓冲已设置: {0}x{1}", width, height);
}

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine