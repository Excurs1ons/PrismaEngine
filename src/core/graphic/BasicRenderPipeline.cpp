#include "BasicRenderPipeline.h"
#include "Logger.h"
#include "GeometryRenderPass.h"

namespace Engine {

BasicRenderPipeline::BasicRenderPipeline()
    : m_renderPipe(nullptr)
{
}

BasicRenderPipeline::~BasicRenderPipeline()
{
    Shutdown();
}

bool BasicRenderPipeline::Initialize(ScriptableRenderPipe* renderPipe)
{
    if (!renderPipe) {
        LOG_ERROR("BasicRenderPipeline", "Invalid render pipe provided");
        return false;
    }

    m_renderPipe = renderPipe;
    
    // 创建几何渲染通道
    m_geometryPass = CreateGeometryPass();
    if (m_geometryPass) {
        m_renderPipe->AddRenderPass(m_geometryPass);
    }
    
    // 创建后期处理通道
    m_postProcessPass = CreatePostProcessPass();
    if (m_postProcessPass) {
        m_renderPipe->AddRenderPass(m_postProcessPass);
    }
    
    LOG_WARNING("BasicRenderPipeline", "Basic render pipeline is deprecated. Please use ForwardPipeline instead.");
    LOG_INFO("BasicRenderPipeline", "Basic render pipeline initialized successfully");
    return true;
}

void BasicRenderPipeline::Shutdown()
{
    if (m_renderPipe) {
        if (m_geometryPass) {
            m_renderPipe->RemoveRenderPass(m_geometryPass);
            m_geometryPass.reset();
        }
        
        if (m_postProcessPass) {
            m_renderPipe->RemoveRenderPass(m_postProcessPass);
            m_postProcessPass.reset();
        }
    }
    
    m_renderPipe = nullptr;
    LOG_INFO("BasicRenderPipeline", "Basic render pipeline shutdown completed");
}

std::shared_ptr<RenderPass> BasicRenderPipeline::CreateGeometryPass()
{
    // 创建几何渲染通道
    // 这里应该创建一个专门处理几何渲染的通道
    auto geometryPass = std::make_shared<GeometryRenderPass>();
    
    LOG_DEBUG("BasicRenderPipeline", "Geometry pass created");
    return geometryPass;
}

std::shared_ptr<RenderPass> BasicRenderPipeline::CreatePostProcessPass()
{
    // 创建后期处理通道
    // 这里应该创建一个专门处理后期处理效果的通道
    // 目前返回空指针，因为后期处理通道还未实现
    LOG_DEBUG("BasicRenderPipeline", "Post-process pass placeholder created");
    return nullptr;
}

} // namespace Engine