#pragma once

#include "graphic/interfaces/IPipeline.h"
#include "graphic/LogicalPass.h"
#include "graphic/LogicalPipeline.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/ICamera.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>
#include <string>

namespace PrismaEngine {
    class UIPass;
}

namespace PrismaEngine::Graphic {

/// @brief 前向渲染管线实现
class ForwardPipeline : public LogicalForwardPipeline, public IPipeline {
public:
    ForwardPipeline();
    virtual ~ForwardPipeline() override;

    // === IPipeline 接口实现 ===
    bool Initialize(IRenderDevice* device) override;
    void Shutdown() override;

    void Execute(const RenderContext& context) override;
    void Begin(const RenderContext& context) override;
    void End() override;

    void SetRenderPassEnabled(const std::string& name, bool enabled) override;
    bool IsRenderPassEnabled(const std::string& name) const override;

    bool AddRenderPass(std::unique_ptr<RenderPass> renderPass, int index = -1) override;
    bool RemoveRenderPass(const std::string& name) override;
    RenderPass* GetRenderPass(const std::string& name) override;
    RenderPass* GetRenderPass(uint32_t index) override;
    std::vector<std::string> GetRenderPassNames() const override;
    uint32_t GetRenderPassCount() const override;

    void SetMainRenderTarget(ITexture* renderTarget, ITexture* depthStencil = nullptr) override;
    ITexture* GetMainRenderTarget() const override;
    ITexture* GetMainDepthStencil() const override;

    bool SetRenderPassDependency(const std::string& srcPass, const std::string& dstPass) override;
    std::vector<std::string> GetRenderPassDependencies(const std::string& name) const override;

    void SetAutoClearRenderTarget(bool clear, const Color& color = {0.0f, 0.0f, 0.0f, 1.0f}) override;
    void SetAutoClearDepthStencil(bool clear, float depth = 1.0f, uint8_t stencil = 0) override;

    const char* GetName() const override { return m_pipelineName.c_str(); }
    void SetName(const std::string& name) override { m_pipelineName = name; }
    void SetDebugMarkersEnabled(bool enabled) override;
    const std::string& GetLastExecutionError() const override { return m_lastError; }

    ExecutionStats GetExecutionStats() const override { return m_stats; }
    void ResetStats() override;

    // === ILogicalPipeline 接口实现 (转发) ===
    void Execute(const PassExecutionContext& context) override;
    void SetViewport(uint32_t width, uint32_t height) override;
    void SetRenderTarget(IRenderTarget* renderTarget) override;
    void SetDepthStencil(IDepthStencil* depthStencil) override;
    IPass* GetPass(size_t index) const override;
    IPass* FindPass(const char* name) const override;
    size_t GetPassCount() const override;
    bool AddPass(IPass* pass) override;
    bool RemovePass(IPass* pass) override;

    // === 额外方法 ===
    void Update(float deltaTime, class ICamera* camera);

    // === Pass 访问 ===
    class DepthPrePass* GetDepthPrePass() const { return m_depthPrePass.get(); }
    class OpaquePass* GetOpaquePass() const { return m_opaquePass.get(); }
    class SkyboxPass* GetSkyboxPass() const { return m_skyboxPass.get(); }
    class TransparentPass* GetTransparentPass() const { return m_transparentPass.get(); }
    class PrismaEngine::UIPass* GetUIPass() const;

private:
    void UpdatePassesCameraData(class ICamera* camera);
    void CollectStats();

private:
    std::shared_ptr<class DepthPrePass> m_depthPrePass;
    std::shared_ptr<class OpaquePass> m_opaquePass;
    std::shared_ptr<class SkyboxPass> m_skyboxPass;
    std::shared_ptr<class TransparentPass> m_transparentPass;
    std::shared_ptr<class PrismaEngine::UIPass> m_uiPass;

    class ICamera* m_camera = nullptr;
    std::string m_pipelineName = "ForwardPipeline";
    std::string m_lastError;
};

} // namespace PrismaEngine::Graphic