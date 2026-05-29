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
#include <array>

namespace Prisma::Graphic {

/// SSAO (Screen-Space Ambient Occlusion) 后处理 Pass
/// 从 G-Buffer 读取位置和法线，使用半球采样计算环境光遮蔽因子
class ENGINE_API SSAOPass {
public:
    SSAOPass();
    ~SSAOPass();

    bool Setup(IRenderDevice* device);
    void Execute(ICommandBuffer* cmd, ITexture* gbPosition, ITexture* gbNormal,
                 ITexture* gbDepth, ITexture* output);
    void Cleanup();

    // 配置参数
    void SetRadius(float radius) { m_radius = radius; }
    float GetRadius() const { return m_radius; }
    void SetPower(float power) { m_power = power; }
    float GetPower() const { return m_power; }
    void SetNumSamples(int samples) { m_numSamples = std::max(1, std::min(samples, 64)); }
    int GetNumSamples() const { return m_numSamples; }
    void SetBlurEnabled(bool enabled) { m_blurEnabled = enabled; }
    bool IsBlurEnabled() const { return m_blurEnabled; }
    bool IsReady() const { return m_ready; }

    // 获取内部 AO 纹理（用于调试或合成）
    ITexture* GetAOTexture() const { return m_aoTexture.get(); }

private:
    bool CreateTextures(uint32_t width, uint32_t height);
    bool CreatePipelines();
    bool CreateDescriptorSets(uint32_t width, uint32_t height);

    // 生成随机半球核 + 噪声纹理
    void GenerateHemisphereSamples();
    void CreateNoiseTexture();

    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    // SSAO 参数
    float m_radius = 0.5f;
    float m_power = 2.0f;
    int m_numSamples = 16;
    bool m_blurEnabled = true;

    // 半球采样核 (CPU 端, 用于初始化)
    std::array<float, 64 * 4> m_hemisphereSamples{}; // vec4 数组, 最多 64 个

    // 着色器
    std::shared_ptr<IShader> m_ssaoShader;
    std::shared_ptr<IShader> m_blurHShader;
    std::shared_ptr<IShader> m_blurVShader;

    // 计算管线
    std::shared_ptr<IComputePipeline> m_ssaoPipeline;
    std::shared_ptr<IComputePipeline> m_blurHPipeline;
    std::shared_ptr<IComputePipeline> m_blurVPipeline;

    // 内部纹理
    std::shared_ptr<ITexture> m_aoTexture;          // R16_Float, 单通道 AO
    std::shared_ptr<ITexture> m_blurTempTexture;    // 临时模糊纹理
    std::shared_ptr<ITexture> m_noiseTexture;       // 4x4 旋转噪声 (RGBA8)

    // 描述符集
    std::shared_ptr<IDescriptorSetLayout> m_ssaoDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_ssaoDescSet;
    std::shared_ptr<IDescriptorSetLayout> m_blurHDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_blurHDescSet;
    std::shared_ptr<IDescriptorSetLayout> m_blurVDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_blurVDescSet;

    // 采样器
    std::shared_ptr<ISampler> m_pointSampler;
    std::shared_ptr<ISampler> m_linearSampler;

    bool m_ready = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
