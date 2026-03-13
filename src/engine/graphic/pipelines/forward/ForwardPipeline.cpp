#include "ForwardPipeline.h"
#include "graphic/RenderPass.h"
#include "pipelines/forward/DepthPrePass.h"
#include "pipelines/forward/OpaquePass.h"
#include "pipelines/SkyboxRenderPass.h"
#include "pipelines/forward/TransparentPass.h"
#include "ui/UIPass.h"
#include "Logger.h"
#include "interfaces/IRenderDevice.h"

namespace PrismaEngine::Graphic {

ForwardPipeline::ForwardPipeline()
    : LogicalForwardPipeline()
    , m_camera(nullptr) {
    m_stats = {};
}

ForwardPipeline::~ForwardPipeline() {
}

// === IPipeline 接口实现 ===

bool ForwardPipeline::Initialize(IRenderDevice* device) {
    m_device = device;
    LOG_INFO("ForwardPipeline", "Initializing ForwardPipeline...");
    
    // 创建所有 Pass
    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_opaquePass = std::make_shared<OpaquePass>();
    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_transparentPass = std::make_shared<TransparentPass>();
    m_uiPass = std::make_shared<PrismaEngine::UIPass>();

    // 添加到 Pipeline
    AddPass(m_depthPrePass.get());
    AddPass(m_opaquePass.get());
    AddPass(m_skyboxPass.get());
    AddPass(m_transparentPass.get());
    AddPass(m_uiPass.get());

    // 启用自动排序
    SetAutoSort(true);

    return true;
}

void ForwardPipeline::Shutdown() {
    m_depthPrePass.reset();
    m_opaquePass.reset();
    m_skyboxPass.reset();
    m_transparentPass.reset();
    m_uiPass.reset();
}

void ForwardPipeline::Execute(const RenderContext& context) {
    PassExecutionContext passContext;
    passContext.deviceContext = nullptr;
    passContext.renderTarget = nullptr;
    passContext.depthStencil = nullptr;
    
    // 调用逻辑 Pipeline 执行
    Execute(passContext);
}

void ForwardPipeline::Begin(const RenderContext& context) {
    (void)context;
}

void ForwardPipeline::End() {
}

void ForwardPipeline::SetRenderPassEnabled(const std::string& name, bool enabled) {
    IPass* pass = FindPass(name.c_str());
    if (pass) pass->SetEnabled(enabled);
}

bool ForwardPipeline::IsRenderPassEnabled(const std::string& name) const {
    IPass* pass = FindPass(name.c_str());
    return pass ? pass->IsEnabled() : false;
}

bool ForwardPipeline::AddRenderPass(std::unique_ptr<RenderPass> renderPass, int index) {
    (void)renderPass; (void)index;
    return false;
}

bool ForwardPipeline::RemoveRenderPass(const std::string& name) {
    (void)name;
    return false;
}

RenderPass* ForwardPipeline::GetRenderPass(const std::string& name) {
    (void)name;
    return nullptr;
}

RenderPass* ForwardPipeline::GetRenderPass(uint32_t index) {
    (void)index;
    return nullptr;
}

std::vector<std::string> ForwardPipeline::GetRenderPassNames() const {
    return {};
}

uint32_t ForwardPipeline::GetRenderPassCount() const {
    return static_cast<uint32_t>(GetPassCount());
}

void ForwardPipeline::SetMainRenderTarget(ITexture* renderTarget, ITexture* depthStencil) {
    (void)renderTarget; (void)depthStencil;
}

ITexture* ForwardPipeline::GetMainRenderTarget() const {
    return nullptr;
}

ITexture* ForwardPipeline::GetMainDepthStencil() const {
    return nullptr;
}

bool ForwardPipeline::SetRenderPassDependency(const std::string& srcPass, const std::string& dstPass) {
    (void)srcPass; (void)dstPass;
    return false;
}

std::vector<std::string> ForwardPipeline::GetRenderPassDependencies(const std::string& name) const {
    (void)name;
    return {};
}

void ForwardPipeline::SetAutoClearRenderTarget(bool clear, const Color& color) {
    (void)clear; (void)color;
}

void ForwardPipeline::SetAutoClearDepthStencil(bool clear, float depth, uint8_t stencil) {
    (void)clear; (void)depth; (void)stencil;
}

void ForwardPipeline::SetDebugMarkersEnabled(bool enabled) {
    m_debugMarkersEnabled = enabled;
}

void ForwardPipeline::ResetStats() {
    m_stats = {};
}

// === ILogicalPipeline 接口实现 (转发到 LogicalPipeline) ===

void ForwardPipeline::Execute(const PassExecutionContext& context) {
    LogicalForwardPipeline::Execute(context);
    CollectStats();
}

void ForwardPipeline::SetViewport(uint32_t width, uint32_t height) {
    LogicalPipeline::SetViewport(width, height);
}

void ForwardPipeline::SetRenderTarget(IRenderTarget* renderTarget) {
    LogicalForwardPipeline::SetRenderTarget(renderTarget);
}

void ForwardPipeline::SetDepthStencil(IDepthStencil* depthStencil) {
    LogicalPipeline::SetDepthStencil(depthStencil);
}

IPass* ForwardPipeline::GetPass(size_t index) const {
    return LogicalPipeline::GetPass(index);
}

IPass* ForwardPipeline::FindPass(const char* name) const {
    return LogicalPipeline::FindPass(name);
}

size_t ForwardPipeline::GetPassCount() const {
    return LogicalPipeline::GetPassCount();
}

bool ForwardPipeline::AddPass(IPass* pass) {
    return LogicalPipeline::AddPass(pass);
}

bool ForwardPipeline::RemovePass(IPass* pass) {
    return LogicalPipeline::RemovePass(pass);
}

// === 额外方法 ===

void ForwardPipeline::Update(float deltaTime, PrismaEngine::Graphic::ICamera* camera) {
    m_camera = camera;
    
    // 更新所有 Pass 的时间
    if (m_depthPrePass) m_depthPrePass->Update(deltaTime);
    if (m_opaquePass) m_opaquePass->Update(deltaTime);
    if (m_skyboxPass) m_skyboxPass->Update(deltaTime);
    if (m_transparentPass) m_transparentPass->Update(deltaTime);
    if (m_uiPass) m_uiPass->Update(deltaTime);

    if (m_camera) {
        UpdatePassesCameraData(m_camera);
    }
}

class PrismaEngine::UIPass* ForwardPipeline::GetUIPass() const {
    return (class PrismaEngine::UIPass*)m_uiPass.get();
}

void ForwardPipeline::UpdatePassesCameraData(PrismaEngine::Graphic::ICamera* camera) {
    if (!camera) return;

    PrismaMath::mat4 view = camera->GetViewMatrix();
    PrismaMath::mat4 projection = camera->GetProjectionMatrix();

    if (m_depthPrePass) {
        m_depthPrePass->SetViewMatrix(view);
        m_depthPrePass->SetProjectionMatrix(projection);
    }
    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(projection);
    }
    if (m_transparentPass) {
        m_transparentPass->SetViewMatrix(view);
        m_transparentPass->SetProjectionMatrix(projection);
    }
    if (m_skyboxPass) {
        PrismaMath::mat4 skyboxView = view;
        skyboxView[3] = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        m_skyboxPass->SetViewMatrix(skyboxView);
        m_skyboxPass->SetProjectionMatrix(projection);
    }
}

void ForwardPipeline::CollectStats() {
    m_stats.drawCalls = 0;
    m_stats.triangles = 0;

    if (m_depthPrePass) {
        const auto& s = m_depthPrePass->GetRenderStats();
        m_stats.drawCalls += s.drawCalls;
        m_stats.triangles += s.triangles;
    }
    if (m_opaquePass) {
        const auto& s = m_opaquePass->GetRenderStats();
        m_stats.drawCalls += s.drawCalls;
        m_stats.triangles += s.triangles;
    }
    if (m_transparentPass) {
        const auto& s = m_transparentPass->GetRenderStats();
        m_stats.drawCalls += s.drawCalls;
        m_stats.triangles += s.triangles;
    }
}

} // namespace PrismaEngine::Graphic