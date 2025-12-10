#include "ScriptableRenderPipeline.h"
#include "Logger.h"

namespace Engine {

ScriptableRenderPipeline::ScriptableRenderPipeline()
    : m_renderBackend(nullptr)
    , m_width(0)
    , m_height(0)
{
}

ScriptableRenderPipeline::~ScriptableRenderPipeline()
{
    Shutdown();
}

bool ScriptableRenderPipeline::Initialize(RenderBackend* renderBackend)
{
    if (!renderBackend) {
        LOG_ERROR("ScriptableRenderPipeline", "无效的渲染后端");
        return false;
    }

    m_renderBackend = renderBackend;
    LOG_INFO("ScriptableRenderPipeline", "渲染管线初始化成功");
    return true;
}

void ScriptableRenderPipeline::Shutdown()
{
    m_renderPasses.clear();
    m_renderBackend = nullptr;
    LOG_INFO("ScriptableRenderPipe", "Scriptable render pipe shutdown completed");
}

void ScriptableRenderPipeline::Execute()
{
    if (!m_renderBackend) {
        LOG_ERROR("ScriptableRenderPipe", "Render backend is not initialized");
        return;
    }

    // 执行所有渲染通道
    for (auto& renderPass : m_renderPasses) {
        if (renderPass) {
            // 创建渲染命令上下文
            // 从渲染后端获取上下文
            auto context = m_renderBackend->CreateCommandContext();
            renderPass->Execute(context);
            
            // 释放上下文
            delete context;
        }
    }
    
    LOG_DEBUG("ScriptableRenderPipe", "Executed {0} render passes", m_renderPasses.size());
}

void ScriptableRenderPipeline::AddRenderPass(std::shared_ptr<RenderPass> renderPass)
{
    if (renderPass) {
        m_renderPasses.push_back(renderPass);
        LOG_DEBUG("ScriptableRenderPipe", "Added render pass. Total passes: {0}", m_renderPasses.size());
    }
}

void ScriptableRenderPipeline::RemoveRenderPass(std::shared_ptr<RenderPass> renderPass)
{
    if (renderPass) {
        auto it = std::find(m_renderPasses.begin(), m_renderPasses.end(), renderPass);
        if (it != m_renderPasses.end()) {
            m_renderPasses.erase(it);
            LOG_DEBUG("ScriptableRenderPipe", "Removed render pass. Total passes: {0}", m_renderPasses.size());
        }
    }
}

void ScriptableRenderPipeline::SetViewportSize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    
    // 通知所有渲染通道视口大小变化
    for (auto& renderPass : m_renderPasses) {
        if (renderPass) {
            renderPass->SetViewport(width, height);
        }
    }
    
    LOG_DEBUG("ScriptableRenderPipe", "Viewport size set to {0}x{1}", width, height);
}

} // namespace Engine