#pragma once

#include "../pipelines/forward/ForwardRenderPassBase.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IRenderTarget.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ITexture;
class IRenderTarget;
class IBuffer;

/**
 * @brief 2D 后处理通道
 * 
 * 提供 Bloom, CRT 滤镜, 灰度, 扭曲等效果。
 * 通过 SetEffect() 动态开启每个效果。
 */
class ENGINE_API PostProcessPass2D : public ForwardRenderPass {
public:
    enum class EffectType {
        None,
        Bloom,
        CRT,
        Grayscale,
        Distortion
    };

    PostProcessPass2D();
    ~PostProcessPass2D() override;

    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    void SetEffect(EffectType type, bool enabled) { m_effects[static_cast<int>(type)] = enabled; }
    bool IsEffectEnabled(EffectType type) const { return m_effects[static_cast<int>(type)]; }

    /**
     * @brief 处理输入纹理并输出到目标
     */
    void Process(ICommandBuffer* cmd, IRenderDevice* device, ITexture* input, IRenderTarget* output = nullptr);

    // ═══ CRT 参数设置 ═══
    void SetCRTIntensity(float v) { m_crtScanlineIntensity = v; }
    float GetCRTIntensity() const { return m_crtScanlineIntensity; }
    void SetCRTAberration(float v) { m_crtChromaticAberration = v; }
    float GetCRTAberration() const { return m_crtChromaticAberration; }
    void SetCRTBrightness(float v) { m_crtBrightness = v; }
    float GetCRTBrightness() const { return m_crtBrightness; }
    void SetCRTContrast(float v) { m_crtContrast = v; }
    float GetCRTContrast() const { return m_crtContrast; }

    // ═══ Grayscale 参数设置 ═══
    void SetGrayscaleIntensity(float v) { m_grayscaleIntensity = v; }
    float GetGrayscaleIntensity() const { return m_grayscaleIntensity; }

    // ═══ Bloom 参数设置 ═══
    void SetBloomIntensity(float v) { m_bloomIntensity = v; }
    float GetBloomIntensity() const { return m_bloomIntensity; }

    // ═══ Distortion 参数设置 ═══
    void SetDistortionSpeed(float v) { m_distortionSpeed = v; }
    float GetDistortionSpeed() const { return m_distortionSpeed; }
    void SetDistortionAmount(float v) { m_distortionAmount = v; }
    float GetDistortionAmount() const { return m_distortionAmount; }

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);
    void EnsureCRTResources(IRenderDevice* device);
    void EnsureGrayscaleResources(IRenderDevice* device);
    void EnsureBloomResources(IRenderDevice* device, uint32_t width, uint32_t height);
    void EnsureDistortionResources(IRenderDevice* device);

    bool m_effects[5] = { false };
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    std::shared_ptr<IPipelineState> m_pso;
    std::shared_ptr<IShader> m_vertShader;
    std::shared_ptr<IShader> m_fragShader;
    std::shared_ptr<IBuffer> m_fullScreenQuad;

    // ═══ CRT 效果资源 ═══
    std::shared_ptr<IPipelineState> m_crtPSO;
    std::shared_ptr<IShader> m_crtVertShader;
    std::shared_ptr<IShader> m_crtFragShader;
    std::shared_ptr<ISampler> m_crtSampler;
    std::shared_ptr<IDescriptorSetLayout> m_crtDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_crtDescSet;

    // CRT 参数默认值
    float m_crtScanlineIntensity = 0.3f;
    float m_crtChromaticAberration = 0.003f;
    float m_crtBrightness = 1.0f;
    float m_crtContrast = 1.0f;

    // ═══ Grayscale 效果资源 ═══
    std::shared_ptr<IPipelineState> m_grayscalePSO;
    std::shared_ptr<IShader> m_grayscaleFragShader;
    std::shared_ptr<ISampler> m_grayscaleSampler;
    std::shared_ptr<IDescriptorSetLayout> m_grayscaleDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_grayscaleDescSet;
    float m_grayscaleIntensity = 1.0f;

    // ═══ Bloom 效果资源 ═══
    std::shared_ptr<IPipelineState> m_bloomPSO;            // Bloom 合成
    std::shared_ptr<IPipelineState> m_bloomGaussianPSO;    // 高斯模糊
    std::shared_ptr<IShader> m_bloomGaussianFragShader;
    std::shared_ptr<IShader> m_bloomCompositeFragShader;
    std::shared_ptr<ITexture> m_bloomTempTexture;
    std::shared_ptr<ISampler> m_bloomSampler;               // 线性采样（用于 Bloom）
    std::shared_ptr<IDescriptorSetLayout> m_bloomDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_bloomDescSet;          // 用于模糊 pass
    std::shared_ptr<IDescriptorSetLayout> m_bloomCompDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_bloomCompDescSet;      // 用于合成 pass
    float m_bloomIntensity = 0.5f;

    // ═══ Distortion 效果资源 ═══
    std::shared_ptr<IPipelineState> m_distortionPSO;
    std::shared_ptr<IShader> m_distortionFragShader;
    std::shared_ptr<ISampler> m_distortionSampler;
    std::shared_ptr<IDescriptorSetLayout> m_distortionDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_distortionDescSet;
    float m_distortionSpeed = 1.0f;
    float m_distortionAmount = 0.02f;
};

} // namespace Prisma::Graphic
