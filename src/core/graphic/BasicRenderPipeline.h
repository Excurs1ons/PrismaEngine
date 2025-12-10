#pragma once

#include "ScriptableRenderPipe.h"
#include "RenderPass.h"
#include <memory>

namespace Engine {

// 基本渲染管线实现 (已弃用)
class BasicRenderPipeline
{
public:
    BasicRenderPipeline();
    ~BasicRenderPipeline();

    // 初始化基本渲染管线
    bool Initialize(ScriptableRenderPipe* renderPipe);
    
    // 关闭渲染管线
    void Shutdown();
    
private:
    // 创建几何渲染通道
    std::shared_ptr<RenderPass> CreateGeometryPass();
    
    // 创建后期处理通道
    std::shared_ptr<RenderPass> CreatePostProcessPass();
    
    // 渲染管线引用
    ScriptableRenderPipe* m_renderPipe;
    
    // 渲染通道
    std::shared_ptr<RenderPass> m_geometryPass;
    std::shared_ptr<RenderPass> m_postProcessPass;
};

} // namespace Engine