#pragma once

#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/Renderer.h"
#include "math/MathTypes.h"
#include "ForwardRenderPassBase.h"
#include <memory>
#include <vector>
#include <unordered_map>

// VkRenderPass 前向声明（离屏管线支持）
typedef struct VkRenderPass_T* VkRenderPass;

namespace Prisma::Graphic {

class ICommandBuffer;
class IShader;
class IPipelineState;

// 透明物体逻辑 Pass
/// 使用深度缓冲和 Alpha 混合渲染透明物体
/// 支持双 PSO 缓存：交换链 PSO + 离屏 PSO
class TransparentPass : public ForwardRenderPass {
public:
    // 渲染统计
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t transparentObjects = 0;
    };

    // PushConstants 结构（对齐 16 字节，用于 MVP + 颜色）
    struct alignas(16) TransparentPushConstants {
        PrismaMath::mat4 mvp;
        Prisma::Color color;
    };

    TransparentPass();
    ~TransparentPass() override = default;

    // === IPass 接口实现 ===

    // 执行 Pass（PassExecutionContext 版本）
    void Execute(const PassExecutionContext& context) override;

    // 执行 Pass（ICommandBuffer 版本，支持完整的材质绑定）
    void Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands);

    // 更新 Pass 数据
    void Update(Prisma::Timestep ts) override;

    // === 渲染设置 ===

    // 设置深度写入
    void SetDepthWrite(bool enable) { m_depthWrite = enable; }

    // 获取深度写入状态
    bool GetDepthWrite() const { return m_depthWrite; }

    // 设置深度测试
    void SetDepthTest(bool enable) { m_depthTest = enable; }

    // 获取深度测试状态
    bool GetDepthTest() const { return m_depthTest; }

    // === 设备设置 ===

    // 设置渲染设备
    void SetDevice(IRenderDevice* device) { m_device = device; }

    // 初始化 Pass（预加载着色器、创建 PSO）
    bool Setup(IRenderDevice* device);

    // === 离屏管线支持 ===

    /// @brief 配置离屏渲染管线 RenderPass
    void CreatePipelineForRenderPass(VkRenderPass rp);

    /// @brief 切换当前使用的管线模式
    void SetUseOffscreenPipeline(bool useOffscreen) { m_useOffscreen = useOffscreen; }

    /// @brief 查询当前是否使用离屏管线
    [[nodiscard]] bool IsUsingOffscreenPipeline() const { return m_useOffscreen; }

    // === 资源清理 ===

    // 释放 Pass 资源
    void Cleanup();

    // === 渲染统计 ===

    // 获取渲染统计
    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }

    // 重置渲染统计
    void ResetStats() { m_stats = RenderStats(); }

private:
    // PSO 创建
    bool EnsureSwapchainPipeline();
    bool EnsureOffscreenPipeline();

    // 向后兼容：默认管线即交换链管线
    bool EnsureDefaultPipeline() { return EnsureSwapchainPipeline(); }

    // 渲染状态
    bool m_depthWrite;
    bool m_depthTest;

    // 渲染统计
    RenderStats m_stats;

    // PSO 缓存
    IRenderDevice* m_device = nullptr;
    std::unordered_map<uint64_t, std::shared_ptr<IPipelineState>> m_psoCache;
    std::shared_ptr<IShader> m_defaultVertexShader;
    std::shared_ptr<IShader> m_defaultPixelShader;

    // 交换链 PSO
    std::shared_ptr<IPipelineState> m_swapchainPipelineState;

    // 离屏 PSO
    std::shared_ptr<IPipelineState> m_offscreenPipelineState;
    bool m_useOffscreen = false;
    VkRenderPass m_offscreenRenderPass = nullptr;

    // 顶点步长（匹配输入布局：POSITION(12) + TEXCOORD(8) + COLOR(16) = 36 字节）
    static constexpr uint32_t kVertexStride = 36;
};

} // namespace Prisma::Graphic
