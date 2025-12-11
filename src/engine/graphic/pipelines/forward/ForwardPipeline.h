#pragma once

#include "graphic/ScriptableRenderPipeline.h"
#include "graphic/pipelines/SkyboxRenderPass.h"
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
    
private:
    // 渲染管线引用
    ScriptableRenderPipeline* m_renderPipe;
    
    // 天空盒渲染通道
    std::shared_ptr<SkyboxRenderPass> m_skyboxRenderPass;
};

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine