#pragma once

#include "../pipelines/forward/ForwardRenderPassBase.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ITexture;
class IRenderTarget;
class IBuffer;

/**
 * @brief 2D 后处理通道
 * 
 * 提供 Bloom, CRT 滤镜, 色调映射等效果。
 * 该类目前处于规划阶段，通过配置文件或代码动态开启。
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

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);
    void EnsureCRTResources(IRenderDevice* device);

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
};

} // namespace Prisma::Graphic
