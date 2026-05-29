#pragma once

#include "Export.h"
#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include <memory>

namespace Prisma::Graphic {

/// TAA (Temporal Anti-Aliasing) 后处理 Pass
/// 使用历史帧累积 + 抖动采样 + AABB 邻域裁剪进行时域抗锯齿
class ENGINE_API TAAPass {
public:
    TAAPass();
    ~TAAPass();

    bool Setup(IRenderDevice* device);
    void Execute(ICommandBuffer* cmd, ITexture* currentFrame, ITexture* motionVectors,
                 ITexture* output);
    void Cleanup();

    // 配置参数
    void SetBlendFactor(float factor) { m_blendFactor = std::max(0.0f, std::min(factor, 0.2f)); }
    float GetBlendFactor() const { return m_blendFactor; }
    void SetJitterScale(float scale) { m_jitterScale = scale; }
    float GetJitterScale() const { return m_jitterScale; }
    void SetSharpness(float sharpness) { m_sharpness = std::max(0.0f, std::min(sharpness, 1.0f)); }
    float GetSharpness() const { return m_sharpness; }
    bool IsReady() const { return m_ready; }

    // 获取历史帧纹理（供下一帧使用）
    ITexture* GetHistoryTexture() const { return m_historyTextureA.get(); }

private:
    bool CreateTextures(uint32_t width, uint32_t height);
    bool CreatePipelines();
    bool CreateDescriptorSets(uint32_t width, uint32_t height);

    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    // TAA 参数
    float m_blendFactor = 0.1f;   // 当前帧混合权重 (0.05-0.1 推荐)
    float m_jitterScale = 1.0f;   // 抖动缩放因子
    float m_sharpness = 0.0f;     // 锐化强度 (0 = 关闭)

    // 着色器
    std::shared_ptr<IShader> m_taaShader;

    // 计算管线
    std::shared_ptr<IComputePipeline> m_taaPipeline;

    // 内部纹理: 历史帧缓冲 (ping-pong)
    std::shared_ptr<ITexture> m_historyTextureA;
    std::shared_ptr<ITexture> m_historyTextureB;
    bool m_historyValid = false;  // 第一帧无有效历史

    // 描述符集
    std::shared_ptr<IDescriptorSetLayout> m_taaDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_taaDescSet;

    // 采样器
    std::shared_ptr<ISampler> m_pointSampler;
    std::shared_ptr<ISampler> m_linearSampler;

    bool m_ready = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
