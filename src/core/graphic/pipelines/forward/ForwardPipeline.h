#pragma once

#include "../../ScriptableRenderPipe.h"
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
    bool Initialize(ScriptableRenderPipe* renderPipe);
    
    // 关闭渲染管线
    void Shutdown();
    
private:
    // 渲染管线引用
    ScriptableRenderPipe* m_renderPipe;
};

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine