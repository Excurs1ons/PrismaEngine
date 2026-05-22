#pragma once

#include "../pipelines/forward/ForwardRenderPassBase.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ITexture;
class IRenderTarget; // Add this one too
class IPipelineState;
class IShader;
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
     * @param cmd 命令缓冲
     * @param device 渲染设备
     * @param input 源纹理
     * @param output 目标渲染目标 (为 nullptr 则输出到交换链)
     */
    void Process(ICommandBuffer* cmd, IRenderDevice* device, ITexture* input, IRenderTarget* output = nullptr);

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);

    bool m_effects[5] = { false };
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    std::shared_ptr<IPipelineState> m_pso;
    std::shared_ptr<IShader> m_vertShader;
    std::shared_ptr<IShader> m_fragShader;
    std::shared_ptr<IBuffer> m_fullScreenQuad;
};

} // namespace Prisma::Graphic
