#pragma once

#include "Export.h"
#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/ShadowMapManager.h"
#include "math/MathTypes.h"
#include "ForwardRenderPassBase.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

// 前向声明
class IRenderTarget;
class IDepthStencil;

// 阴影 Pass
/// 渲染方向光的阴影贴图 (CSM)，每光源最多 4 个级联
/// 继承 ForwardRenderPass 以使用相机矩阵基础设施
class ENGINE_API ShadowPass : public ForwardRenderPass {
public:
    // PushConstants 结构（对齐 16 字节）
    struct alignas(16) ShadowPushConstants {
        PrismaMath::mat4 lightVP;       // 光照空间视投影矩阵
        float shadowBias;                // 深度偏移
        float shadowNormalBias;          // 法线偏移
        uint32_t cascadeIndex;           // 当前级联索引
        float cascadeCount;              // 总级联数
    };

    ShadowPass();
    ~ShadowPass() override;

    // === IPass 接口实现 ===

    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    // === 生命周期 ===

    /// @brief 初始化 Pass
    /// @param device 渲染设备
    /// @param factory 资源工厂
    /// @return 初始化是否成功
    bool Setup(IRenderDevice* device, IResourceFactory* factory);

    /// @brief 释放 Pass 资源
    void Cleanup();

    // === 执行 ===

    /// @brief 使用指定光源列表执行阴影渲染
    /// @param cmd 命令缓冲区
    /// @param lights 阴影光源列表
    void Execute(ICommandBuffer* cmd, const std::vector<ShadowLight>& lights);

    // === ShadowMapManager 集成 ===

    /// @brief 设置阴影贴图管理器
    void SetShadowMapManager(ShadowMapManager* manager) { m_shadowMapManager = manager; }

    /// @brief 获取阴影贴图管理器
    ShadowMapManager* GetShadowMapManager() const { return m_shadowMapManager; }

    // === 级联配置 ===

    /// @brief 设置级联数量 (1~4)
    void SetCascadeCount(uint32_t count) {
        m_cascadeCount = std::min(count, ShadowMapManager::kMaxCascades);
    }

    /// @brief 获取级联数量
    uint32_t GetCascadeCount() const { return m_cascadeCount; }

    // === PCF 配置 ===

    /// @brief 设置 PCF 内核大小 (采样数)
    void SetPCFKernelSize(uint32_t size) { m_pcfKernelSize = size; }

    /// @brief 获取 PCF 内核大小
    uint32_t GetPCFKernelSize() const { return m_pcfKernelSize; }

    // === 状态查询 ===

    /// @brief Pass 是否已准备就绪
    bool IsReady() const { return m_ready; }

private:
    // 创建阴影深度 PSO
    bool EnsurePipeline();

    // 渲染一个级联的阴影贴图
    void RenderCascade(ICommandBuffer* cmd, const ShadowLight& light,
                       const PrismaMath::mat4& lightVP, uint32_t cascadeIndex,
                       const std::vector<RenderCommand>& geometry);

    // 阴影贴图管理器 (不拥有)
    ShadowMapManager* m_shadowMapManager = nullptr;

    // 级联数量
    uint32_t m_cascadeCount = 4;

    // PCF 内核大小
    uint32_t m_pcfKernelSize = 4;

    // 设备/工厂指针
    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    // 着色器
    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_pixelShader;

    // PSO
    std::shared_ptr<IPipelineState> m_pipelineState;

    // 就绪标志
    bool m_ready = false;
};

} // namespace Prisma::Graphic
