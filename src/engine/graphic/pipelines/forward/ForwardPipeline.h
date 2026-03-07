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

namespace PrismaEngine {
    class UIPass;
}

namespace PrismaEngine::Graphic {

/// @brief 前向渲染管线实现
class ForwardPipeline : public LogicalForwardPipeline {
public:
    ForwardPipeline();
    ~ForwardPipeline() override;

    bool Initialize();
    void Update(float deltaTime, class ICamera* camera);
    void Execute(const PassExecutionContext& context) override;

    // === Pass 访问 ===
    class DepthPrePass* GetDepthPrePass() const { return m_depthPrePass.get(); }
    class OpaquePass* GetOpaquePass() const { return m_opaquePass.get(); }
    class SkyboxPass* GetSkyboxPass() const { return m_skyboxPass.get(); }
    class TransparentPass* GetTransparentPass() const { return m_transparentPass.get(); }
    class PrismaEngine::UIPass* GetUIPass() const { return m_uiPass.get(); }

    // === 渲染统计 ===
    struct RenderStats {
        uint32_t totalDrawCalls = 0;
        uint32_t totalTriangles = 0;
        uint32_t opaqueObjects = 0;
        uint32_t transparentObjects = 0;
        float lastFrameTime = 0.0f;
    };
    const RenderStats& GetRenderStats() const { return m_stats; }

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
    RenderStats m_stats;
};

} // namespace PrismaEngine::Graphic