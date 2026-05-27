#pragma once

#include "../pipelines/forward/ForwardRenderPassBase.h"
#include "interfaces/RenderTypes.h"
#include <memory>

namespace Prisma::Graphic {

class ITexture;
class IShader;
class IPipelineState;
class IBuffer;
class IRenderDevice;
class ICommandBuffer;
class IDescriptorSet;
class IDescriptorSetLayout;

/**
 * @brief 2D 屏幕空间反射渲染通道
 *
 * 在场景渲染完成后，使用前一帧捕获的场景颜色纹理(ReflectionSource)，
 * 对反射材质表面采样镜像UV坐标，实现水面倒影/镜面效果。
 *
 * 工作流程:
 *   1. 每帧结束时 CaptureScene() 将当前 swapchain 颜色拷贝到 m_reflectionSource
 *   2. 下一帧反射表面采样 m_reflectionSource 以实现 SSR
 *   3. 反射参数通过 UBO (binding=3) 传递
 */
class ENGINE_API ReflectionPass2D : public ForwardRenderPass {
public:
    ReflectionPass2D();
    ~ReflectionPass2D() override;

    // IPass 接口
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    // 执行反射渲染 (由 ForwardPipeline 调用)
    void ExecuteReflection(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height);

    // 捕获当前场景颜色到反射源纹理 (帧结束时调用)
    void CaptureScene(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height);

    // 获取反射源纹理
    std::shared_ptr<ITexture> GetReflectionSource() const { return m_reflectionSource; }

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    // === UBO 参数设置 ===

    void SetReflectOffset(const PrismaMath::vec2& offset) { m_uboData.reflectOffset = offset; }
    void SetReflectStrength(float strength) { m_uboData.reflectStrength = strength; }
    void SetFadeDistance(float distance) { m_uboData.fadeDistance = distance; }
    void SetDistortion(float distortion) { m_uboData.distortion = distortion; }
    void SetTintColor(const PrismaMath::vec4& tint) { m_uboData.tintColor = tint; }

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);

    void CreateReflectionPSO(IRenderDevice* device);

    // UBO 数据结构 (匹配 GLSL 中的 ReflectionUBO, std140 对齐)
    struct alignas(16) ReflectionUBOData {
        alignas(8)  PrismaMath::vec2 reflectOffset   = {0.0f, 1.0f};  // 默认垂直翻转
        alignas(4)  float reflectStrength             = 0.5f;
        alignas(4)  float fadeDistance                = 2.0f;
        alignas(4)  float distortion                  = 0.0f;  // v1 未使用
        alignas(16) PrismaMath::vec4 tintColor        = {1.0f, 1.0f, 1.0f, 1.0f};
    };

    std::shared_ptr<ITexture> m_reflectionSource;
    std::shared_ptr<IShader> m_vertexShader;       // Renderer2D.vert.spv (reused)
    std::shared_ptr<IShader> m_fragmentShader;      // ReflectionSprite.frag.spv
    std::shared_ptr<IPipelineState> m_reflectionPSO;
    std::shared_ptr<IBuffer> m_reflectionUBO;       // UBO buffer for reflection params

    // 描述符集: binding 0=AlbedoMap, 1=ReflectionSource, 3=UBO
    std::shared_ptr<IDescriptorSetLayout> m_reflectionDSLayout;
    std::shared_ptr<IDescriptorSet> m_reflectionDS;

    ReflectionUBOData m_uboData;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_enabled = false; // 默认禁用，因为 CPU 回读严重影响性能
    float m_logTimer = 5.0f; // 初始为 5.0，确保第一次能打 log
};

} // namespace Prisma::Graphic
