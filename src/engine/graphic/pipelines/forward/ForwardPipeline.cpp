#include "ForwardPipeline.h"
#include "graphic/pipelines/SkyboxRenderPass.h"
#include "Logger.h"

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

ForwardPipeline::ForwardPipeline()
    : m_renderPipe(nullptr)
{
    LOG_DEBUG("ForwardPipeline", "构造函数被调用");
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
    
    // 创建天空盒渲染通道
    LOG_DEBUG("ForwardPipeline", "创建天空盒渲染通道");
    m_skyboxRenderPass = std::make_shared<SkyboxRenderPass>();
    m_renderPipe->AddRenderPass(m_skyboxRenderPass);
    
    LOG_INFO("ForwardPipeline", "Forward rendering pipeline initialized successfully");
    return true;
}

void ForwardPipeline::Shutdown()
{
    LOG_DEBUG("ForwardPipeline", "关闭前向渲染管线");
    m_skyboxRenderPass.reset();
    m_renderPipe = nullptr;
    LOG_INFO("ForwardPipeline", "Forward rendering pipeline shutdown completed");
}

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine