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
}

ForwardPipeline::~ForwardPipeline()
{
    Shutdown();
}

bool ForwardPipeline::Initialize(ScriptableRenderPipeline* renderPipe)
{
    if (!renderPipe) {
        LOG_ERROR("ForwardPipeline", "Invalid render pipe provided");
        return false;
    }

    m_renderPipe = renderPipe;
    
    // 创建天空盒渲染通道
    m_skyboxRenderPass = std::make_shared<SkyboxRenderPass>();
    m_renderPipe->AddRenderPass(m_skyboxRenderPass);
    
    LOG_INFO("ForwardPipeline", "Forward rendering pipeline initialized successfully");
    return true;
}

void ForwardPipeline::Shutdown()
{
    m_skyboxRenderPass.reset();
    m_renderPipe = nullptr;
    LOG_INFO("ForwardPipeline", "Forward rendering pipeline shutdown completed");
}

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine