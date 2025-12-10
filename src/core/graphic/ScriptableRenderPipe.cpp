#include "ScriptableRenderPipe.h"
#include "Logger.h"

namespace Engine {

ScriptableRenderPipe::ScriptableRenderPipe()
    : m_renderBackend(nullptr)
    , m_width(0)
    , m_height(0)
{
}

ScriptableRenderPipe::~ScriptableRenderPipe()
{
    Shutdown();
}

bool ScriptableRenderPipe::Initialize(RenderBackend* renderBackend)
{
    if (!renderBackend) {
        LOG_ERROR("ScriptableRenderPipe", "Invalid render backend provided");
        return false;
    }

    m_renderBackend = renderBackend;
    LOG_INFO("ScriptableRenderPipe", "Scriptable render pipe initialized successfully");
    return true;
}

void ScriptableRenderPipe::Shutdown()
{
    m_renderPasses.clear();
    m_renderBackend = nullptr;
    LOG_INFO("ScriptableRenderPipe", "Scriptable render pipe shutdown completed");
}

void ScriptableRenderPipe::Execute()
{
    if (!m_renderBackend) {
        LOG_ERROR("ScriptableRenderPipe", "Render backend is not initialized");
        return;
    }

    // 执行所有渲染通道
    for (auto& renderPass : m_renderPasses) {
        if (renderPass) {
            // 创建渲染命令上下文
            // 注意：这里需要根据具体的渲染后端创建相应的上下文
            // 由于上下文创建依赖于具体后端实现，暂时传空指针
            renderPass->Execute(nullptr);
        }
    }
    
    LOG_DEBUG("ScriptableRenderPipe", "Executed {0} render passes", m_renderPasses.size());
}

void ScriptableRenderPipe::AddRenderPass(std::shared_ptr<RenderPass> renderPass)
{
    if (renderPass) {
        m_renderPasses.push_back(renderPass);
        LOG_DEBUG("ScriptableRenderPipe", "Added render pass. Total passes: {0}", m_renderPasses.size());
    }
}

void ScriptableRenderPipe::RemoveRenderPass(std::shared_ptr<RenderPass> renderPass)
{
    if (renderPass) {
        auto it = std::find(m_renderPasses.begin(), m_renderPasses.end(), renderPass);
        if (it != m_renderPasses.end()) {
            m_renderPasses.erase(it);
            LOG_DEBUG("ScriptableRenderPipe", "Removed render pass. Total passes: {0}", m_renderPasses.size());
        }
    }
}

void ScriptableRenderPipe::SetViewportSize(uint32_t width, uint32_t height)
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