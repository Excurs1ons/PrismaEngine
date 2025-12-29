//#include "ScriptableRenderPipeline.h"
//#include "RenderAPI.h"
//#include "RenderCommandContext.h"
//#include "Logger.h"
//// TODO: 更新以使用新架构
//
//using namespace PrismaEngine::Graphic;
//
//ScriptableRenderPipeline::ScriptableRenderPipeline()
//    : m_renderBackend(nullptr)
//    , m_width(0)
//    , m_height(0)
//{
//    LOG_DEBUG("ScriptableRenderPipeline", "构造函数被调用");
//}
//
//ScriptableRenderPipeline::~ScriptableRenderPipeline()
//{
//    LOG_DEBUG("ScriptableRenderPipeline", "析构函数被调用");
//    Shutdown();
//}
//
//bool ScriptableRenderPipeline::Initialize(RenderAPI* renderBackend)
//{
//    LOG_DEBUG("ScriptableRenderPipeline", "初始化渲染管线");
//
//    if (!renderBackend) {
//        LOG_ERROR("ScriptableRenderPipeline", "无效的渲染后端");
//        return false;
//    }
//
//    m_renderBackend = renderBackend;
//    LOG_INFO("ScriptableRenderPipeline", "渲染管线初始化成功");
//    return true;
//}
//
//void ScriptableRenderPipeline::Shutdown()
//{
//    LOG_DEBUG("ScriptableRenderPipeline", "关闭渲染管线");
//
//    // 清理缓存的命令上下文
//    if (m_cachedContext) {
//        delete m_cachedContext;
//        m_cachedContext = nullptr;
//    }
//
//    m_renderPasses.clear();
//    m_renderBackend = nullptr;
//    LOG_INFO("ScriptableRenderPipe", "Scriptable render pipe shutdown completed");
//}
//
//void ScriptableRenderPipeline::Execute()
//{
//    LOG_DEBUG("ScriptableRenderPipeline", "开始执行渲染管线，渲染通道数量: {0}", m_renderPasses.size());
//
//    if (!m_renderBackend) {
//        LOG_ERROR("ScriptableRenderPipe", "Render backend is not initialized");
//        return;
//    }
//
//    // 每帧创建新的命令上下文
//    // 这样确保命令列表的状态是正确的
//    if (m_cachedContext) {
//        delete m_cachedContext;
//        m_cachedContext = nullptr;
//    }
//
//    m_cachedContext = static_cast<RenderCommandContext*>(m_renderBackend->CreateCommandContext());
//    if (!m_cachedContext) {
//        LOG_ERROR("ScriptableRenderPipeline", "无法创建命令上下文");
//        return;
//    }
//    LOG_DEBUG("ScriptableRenderPipeline", "创建命令上下文: 0x{0:x}", reinterpret_cast<uintptr_t>(m_cachedContext));
//
//    // 执行所有渲染通道
//    for (size_t i = 0; i < m_renderPasses.size(); ++i) {
//        auto& renderPass = m_renderPasses[i];
//        if (renderPass) {
//            LOG_DEBUG("ScriptableRenderPipeline", "执行第 {0} 个渲染通道", i);
//            renderPass->Execute(m_cachedContext);
//        }
//    }
//
//    LOG_DEBUG("ScriptableRenderPipeline", "所有渲染通道执行完成");
//
//    LOG_DEBUG("ScriptableRenderPipeline", "渲染管线执行完成，共执行 {0} 个渲染通道", m_renderPasses.size());
//}
//
//void ScriptableRenderPipeline::AddRenderPass(std::shared_ptr<RenderPass> renderPass)
//{
//    if (renderPass) {
//        m_renderPasses.push_back(renderPass);
//        LOG_DEBUG("ScriptableRenderPipe", "添加渲染通道. 总数: {0}", m_renderPasses.size());
//    }
//}
//
//void ScriptableRenderPipeline::RemoveRenderPass(std::shared_ptr<RenderPass> renderPass)
//{
//    if (renderPass) {
//        auto it = std::find(m_renderPasses.begin(), m_renderPasses.end(), renderPass);
//        if (it != m_renderPasses.end()) {
//            m_renderPasses.erase(it);
//            LOG_DEBUG("ScriptableRenderPipe", "移除渲染通道. 总数: {0}", m_renderPasses.size());
//        }
//    }
//}
//
//void ScriptableRenderPipeline::SetViewportSize(uint32_t width, uint32_t height)
//{
//    LOG_DEBUG("ScriptableRenderPipeline", "设置视口大小为 {0}x{1}", width, height);
//
//    m_width = width;
//    m_height = height;
//
//    // 通知所有渲染通道视口大小变化
//    for (auto& renderPass : m_renderPasses) {
//        if (renderPass) {
//            renderPass->SetViewport(width, height);
//        }
//    }
//
//    LOG_DEBUG("ScriptableRenderPipe", "Viewport size set to {0}x{1}", width, height);
//}