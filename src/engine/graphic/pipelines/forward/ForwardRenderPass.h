#pragma once

#include "graphic/RenderPass.h"

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

// 前向渲染通道
class ForwardRenderPass : public RenderPass
{
public:
    ForwardRenderPass();
    ~ForwardRenderPass();
    
    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;
    
    // 设置渲染目标
    void SetRenderTarget(void* renderTarget) override;
    
    // 清屏操作
    void ClearRenderTarget(float r, float g, float b, float a) override;
    
    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;
};

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine