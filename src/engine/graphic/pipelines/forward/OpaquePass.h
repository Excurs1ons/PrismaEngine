#pragma once

#include "interfaces/RenderTypes.h"
#include "graphic/Renderer.h"
#include "ForwardRenderPassBase.h"
#include <memory>
#include <vector>
#include <unordered_map>

// VkRenderPass 前向声明（离屏管线需要存储外部传入的 RenderPass）
typedef struct VkRenderPass_T* VkRenderPass;

namespace Prisma::Graphic {

class ICommandBuffer;
class IShader;
class IPipelineState;

/* 不透明渲染通道 (Opaque Pass)
 *
 * 支持双 PSO 缓存：交换链 PSO + 离屏 PSO
 * - 交换链 PSO：用于 ForwardPipeline（默认路径）
 * - 离屏 PSO：用于 Pipeline2D PixelPerfect 模式（离屏 RT → swapchain blit）
 *
 * 这解决了 Bug 5：OpaquePass PSO 创建时绑定 Swapchain RP，
 * 但 Pipeline2D 在 PixelPerfect 模式下激活的是 Offscreen RP，
 * 两者 RenderPass 格式/dependency 不兼容导致 Vulkan 静默拒绝绘制。
 */
class OpaquePass : public ForwardRenderPass {
public:
    OpaquePass();
    ~OpaquePass() override = default;

    // IPass 接口实现
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    // 旧版执行接口支持 (向后兼容)
    void Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands);

    // 数据设置
    void SetLights(const std::vector<Light>& lights);
    void SetDevice(IRenderDevice* device) { m_device = device; }

    // ── 离屏管线支持（Bug 5 修复） ──
    /// @brief 配置离屏渲染管线 RenderPass（在 Initialize 阶段调用一次）
    void CreatePipelineForRenderPass(VkRenderPass rp);

    /// @brief 切换当前使用的管线模式
    void SetUseOffscreenPipeline(bool useOffscreen) { m_useOffscreen = useOffscreen; }

    /// @brief 查询当前是否使用离屏管线
    [[nodiscard]] bool IsUsingOffscreenPipeline() const { return m_useOffscreen; }

private:
    // 向后兼容：默认管线即交换链管线
    bool EnsureDefaultPipeline() { return EnsureSwapchainPipeline(); }

    bool EnsureSwapchainPipeline();
    bool EnsureOffscreenPipeline();

    std::vector<Light> m_Lights;
    IRenderDevice* m_device = nullptr;
    std::unordered_map<uint64_t, std::shared_ptr<IPipelineState>> m_psoCache;
    std::shared_ptr<IShader> m_defaultVertexShader;
    std::shared_ptr<IShader> m_defaultPixelShader;

    // 交换链 PSO（原 m_defaultPipelineState）
    std::shared_ptr<IPipelineState> m_swapchainPipelineState;

    // 离屏 PSO（Bug 5 修复：用于 Pipeline2D PixelPerfect 模式）
    std::shared_ptr<IPipelineState> m_offscreenPipelineState;
    bool m_useOffscreen = false;
    VkRenderPass m_offscreenRenderPass = nullptr;
};

} // namespace Prisma::Graphic
