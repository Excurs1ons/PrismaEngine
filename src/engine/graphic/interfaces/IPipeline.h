#pragma once

#include "RenderTypes.h"
#include <memory>
#include <string>
#include <vector>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;
class ICommandBuffer;
class RenderPass;

/// @brief 渲染上下文
struct RenderContext {
    /// @brief 渲染设备
    IRenderDevice* device = nullptr;

    /// @brief 主命令缓冲区
    ICommandBuffer* commandBuffer = nullptr;

    /// @brief 场景数据（如相机信息）
    void* sceneData = nullptr;

    /// @brief 当前帧索引
    uint32_t frameIndex = 0;

    /// @brief 时间信息
    float deltaTime = 0.0f;
    float totalTime = 0.0f;

    /// @brief 渲染目标尺寸
    uint32_t renderTargetWidth = 0;
    uint32_t renderTargetHeight = 0;

    /// @brief 用户数据
    void* userData = nullptr;
};

/// @brief 渲染流程控制器抽象接口
/// 管理和执行一系列渲染通道(RenderPass)
class IPipeline {
public:
    virtual ~IPipeline() = default;

    /// @brief 初始化渲染流程
    /// @param device 渲染设备
    /// @return 是否初始化成功
    virtual bool Initialize(IRenderDevice* device) = 0;

    /// @brief 关闭渲染流程
    virtual void Shutdown() = 0;

    // === 渲染通道管理 ===

    /// @brief 添加渲染通道
    /// @param renderPass 渲染通道
    /// @param index 插入位置（-1表示添加到末尾）
    /// @return 是否添加成功
    virtual bool AddRenderPass(std::unique_ptr<RenderPass> renderPass, int index = -1) = 0;

    /// @brief 移除渲染通道
    /// @param name 渲染通道名称
    /// @return 是否移除成功
    virtual bool RemoveRenderPass(const std::string& name) = 0;

    /// @brief 获取渲染通道
    /// @param name 渲染通道名称
    /// @return 渲染通道指针（如果未找到返回nullptr）
    virtual RenderPass* GetRenderPass(const std::string& name) = 0;

    /// @brief 获取渲染通道
    /// @param index 渲染通道索引
    /// @return 渲染通道指针（如果索引无效返回nullptr）
    virtual RenderPass* GetRenderPass(uint32_t index) = 0;

    /// @brief 获取所有渲染通道名称
    /// @return 名称数组
    virtual std::vector<std::string> GetRenderPassNames() const = 0;

    /// @brief 获取渲染通道数量
    /// @return 数量
    virtual uint32_t GetRenderPassCount() const = 0;

    // === 执行控制 ===

    /// @brief 执行渲染流程
    /// @param context 渲染上下文
    virtual void Execute(const RenderContext& context) = 0;

    /// @brief 开始渲染流程
    /// @param context 渲染上下文
    virtual void Begin(const RenderContext& context) = 0;

    /// @brief 结束渲染流程
    virtual void End() = 0;

    /// @brief 设置是否启用某个渲染通道
    /// @param name 渲染通道名称
    /// @param enabled 是否启用
    virtual void SetRenderPassEnabled(const std::string& name, bool enabled) = 0;

    /// @brief 检查渲染通道是否启用
    /// @param name 渲染通道名称
    /// @return 是否启用
    virtual bool IsRenderPassEnabled(const std::string& name) const = 0;

    // === 渲染目标管理 ===

    /// @brief 设置主渲染目标
    /// @param renderTarget 渲染目标纹理
    /// @param depthStencil 深度模板缓冲区
    virtual void SetMainRenderTarget(ITexture* renderTarget, ITexture* depthStencil = nullptr) = 0;

    /// @brief 获取主渲染目标
    /// @return 渲染目标纹理
    virtual ITexture* GetMainRenderTarget() const = 0;

    /// @brief 获取主深度模板缓冲区
    /// @return 深度模板缓冲区
    virtual ITexture* GetMainDepthStencil() const = 0;

    // === 依赖关系 ===

    /// @brief 设置渲染通道之间的依赖关系
    /// @param srcPass 源通道名称
    /// @param dstPass 目标通道名称
    /// @return 是否设置成功
    virtual bool SetRenderPassDependency(const std::string& srcPass, const std::string& dstPass) = 0;

    /// @brief 获取渲染通道的依赖
    /// @param name 渲染通道名称
    /// @return 依赖的渲染通道名称数组
    virtual std::vector<std::string> GetRenderPassDependencies(const std::string& name) const = 0;

    // === 配置 ===

    /// @brief 设置是否自动清除渲染目标
    /// @param clear 是否清除
    /// @param color 清除颜色
    virtual void SetAutoClearRenderTarget(bool clear, const Color& color = {0.0f, 0.0f, 0.0f, 1.0f}) = 0;

    /// @brief 设置是否自动清除深度模板
    /// @param clear 是否清除
    /// @param depth 深度清除值
    /// @param stencil 模板清除值
    virtual void SetAutoClearDepthStencil(bool clear, float depth = 1.0f, uint8_t stencil = 0) = 0;

    // === 调试 ===

    /// @brief 获取渲染流程名称
    /// @return 名称
    virtual const std::string& GetName() const = 0;

    /// @brief 设置渲染流程名称
    /// @param name 名称
    virtual void SetName(const std::string& name) = 0;

    /// @brief 启用/禁用调试标记
    /// @param enabled 是否启用
    virtual void SetDebugMarkersEnabled(bool enabled) = 0;

    /// @brief 获取上一次执行的错误信息
    /// @return 错误信息
    virtual const std::string& GetLastExecutionError() const = 0;

    // === 统计 ===

    /// @brief 获取执行统计
    struct ExecutionStats {
        uint32_t totalPasses = 0;
        uint32_t executedPasses = 0;
        uint32_t skippedPasses = 0;
        float executionTime = 0.0f;  // 毫秒
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
    };

    /// @brief 获取执行统计
    /// @return 统计信息
    virtual ExecutionStats GetExecutionStats() const = 0;

    /// @brief 重置统计信息
    virtual void ResetStats() = 0;

protected:
    std::string m_name;
    bool m_debugMarkersEnabled = false;
    mutable ExecutionStats m_stats = {};
    mutable std::string m_lastError;
};

} // namespace PrismaEngine::Graphic