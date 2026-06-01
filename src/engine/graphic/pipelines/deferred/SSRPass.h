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

/// SSR (Screen-Space Reflections) 后处理 Pass
/// 从 G-buffer 读取颜色/法线/深度/位置，
/// 沿反射方向在屏幕空间进行光线步进，
/// 输出反射颜色 + 置信度 (alpha 通道)。
class ENGINE_API SSRPass {
public:
    SSRPass();
    ~SSRPass();

    bool Setup(IRenderDevice* device);
    void Execute(ICommandBuffer* cmd, ITexture* gbColor, ITexture* gbNormal,
                 ITexture* gbDepth, ITexture* gbPosition, ITexture* output);
    void Cleanup();

    // 配置参数
    void SetStepCount(float count) { m_stepCount = count; }
    float GetStepCount() const { return m_stepCount; }
    void SetThickness(float thickness) { m_thickness = thickness; }
    float GetThickness() const { return m_thickness; }
    void SetMaxDistance(float dist) { m_maxDistance = dist; }
    float GetMaxDistance() const { return m_maxDistance; }
    void SetFadeFactor(float fade) { m_fadeFactor = fade; }
    float GetFadeFactor() const { return m_fadeFactor; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    bool IsReady() const { return m_ready; }

    // 获取 SSR 反射纹理（供合成使用）
    ITexture* GetReflectionTexture() const { return m_reflectionTexture.get(); }

private:
    bool CreateTextures(uint32_t width, uint32_t height);
    bool CreatePipelines();
    bool CreateDescriptorSets(uint32_t width, uint32_t height);

    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    // SSR 参数
    float m_stepCount = 25.0f;
    float m_thickness = 0.1f;
    float m_maxDistance = 50.0f;
    float m_fadeFactor = 0.1f;
    bool m_enabled = true;

    // 着色器
    std::shared_ptr<IShader> m_ssrShader;

    // 计算管线
    std::shared_ptr<IComputePipeline> m_ssrPipeline;

    // 内部反射纹理 (RGBA16F, 存储反射颜色 + 置信度 alpha)
    std::shared_ptr<ITexture> m_reflectionTexture;

    // 描述符集
    std::shared_ptr<IDescriptorSetLayout> m_ssrDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_ssrDescSet;

    // 采样器
    std::shared_ptr<ISampler> m_pointSampler;
    std::shared_ptr<ISampler> m_linearSampler;

    bool m_ready = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
