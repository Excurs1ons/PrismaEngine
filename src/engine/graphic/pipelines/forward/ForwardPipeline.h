#pragma once

#include "graphic/ScriptableRenderPipeline.h"
#include "graphic/pipelines/SkyboxRenderPass.h"
#include "graphic/ICamera.h"
#include "DepthPrePass.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include <memory>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

// 前向渲染管线
class ForwardPipeline
{
public:
    ForwardPipeline();
    ~ForwardPipeline();

    // 初始化前向渲染管线
    bool Initialize(ScriptableRenderPipeline* renderPipe);

    // 关闭渲染管线
    void Shutdown();

    // 更新渲染管线（每帧调用）
    void Update(float deltaTime);

    // 设置相机
    void SetCamera(ICamera* camera);

private:
    // 渲染管线引用
    ScriptableRenderPipeline* m_renderPipe;

    // 渲染通道
    std::shared_ptr<SkyboxRenderPass> m_skyboxRenderPass;
    std::shared_ptr<DepthPrePass> m_depthPrePass;
    std::shared_ptr<OpaquePass> m_opaquePass;
    std::shared_ptr<TransparentPass> m_transparentPass;

    // 相机引用
    ICamera* m_camera;

    // 渲染统计
    struct RenderStats {
        uint32_t depthPrePassObjects = 0;
        uint32_t opaquePassObjects = 0;
        uint32_t transparentPassObjects = 0;
        float lastFrameTime = 0.0f;
    };
    RenderStats m_stats;
};

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine