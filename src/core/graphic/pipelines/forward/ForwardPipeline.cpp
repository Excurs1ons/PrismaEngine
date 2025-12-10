#include "ForwardPipeline.h"
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
    
    LOG_INFO("ForwardPipeline", "Forward rendering pipeline initialized successfully");
    return true;
}

void ForwardPipeline::Shutdown()
{
    m_renderPipe = nullptr;
    LOG_INFO("ForwardPipeline", "Forward rendering pipeline shutdown completed");
}

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine