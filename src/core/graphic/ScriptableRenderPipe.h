#pragma once

#include "RenderPass.h"
#include "RenderBackend.h"
#include <vector>
#include <memory>

namespace Engine {

// 可编程渲染管线类
class ScriptableRenderPipe
{
public:
    ScriptableRenderPipe();
    ~ScriptableRenderPipe();

    // 初始化渲染管线
    bool Initialize(RenderBackend* renderBackend);
    
    // 关闭渲染管线
    void Shutdown();
    
    // 执行渲染管线
    void Execute();
    
    // 添加渲染通道
    void AddRenderPass(std::shared_ptr<RenderPass> renderPass);
    
    // 移除渲染通道
    void RemoveRenderPass(std::shared_ptr<RenderPass> renderPass);
    
    // 获取渲染后端
    RenderBackend* GetRenderBackend() const { return m_renderBackend; }
    
    // 设置视口大小
    void SetViewportSize(uint32_t width, uint32_t height);

private:
    // 渲染后端引用
    RenderBackend* m_renderBackend;
    
    // 渲染通道列表
    std::vector<std::shared_ptr<RenderPass>> m_renderPasses;
    
    // 视口尺寸
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace Engine