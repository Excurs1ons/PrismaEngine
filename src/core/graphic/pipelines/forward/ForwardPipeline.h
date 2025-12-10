#pragma once

#include "graphic/ScriptableRenderPipeline.h"
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
};

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine