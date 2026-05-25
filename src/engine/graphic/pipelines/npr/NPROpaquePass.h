#pragma once

#include "interfaces/RenderTypes.h"
#include "graphic/Renderer.h"
#include "../forward/ForwardRenderPassBase.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ICommandBuffer;
class IShader;
class IPipelineState;
class IDescriptorSet;
class IDescriptorSetLayout;
class IBuffer;

/**
 * @brief NPR 不透明渲染通道 (NPR Opaque Pass)
 * 使用 npr_lit.vert/frag 着色器进行 NPR 风格的 Toon/Cel 渲染。
 * 没有任何单例，由 NPRPipeline 调用。
 */
class NPROpaquePass : public ForwardRenderPass {
public:
    NPROpaquePass();
    ~NPROpaquePass() override = default;

    // IPass 接口实现
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    // 旧版执行接口 (向后兼容)
    void Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands);

    // 数据设置
    void SetLights(const std::vector<Light>& lights);
    void SetDevice(IRenderDevice* device) { m_device = device; }

private:
    bool EnsureDefaultPipeline();

    std::vector<Light> m_Lights;
    IRenderDevice* m_device = nullptr;
    std::shared_ptr<IShader> m_defaultVertexShader;
    std::shared_ptr<IShader> m_defaultPixelShader;
    std::shared_ptr<IPipelineState> m_defaultPipelineState;

    // Set 0: NPR 材质数据 (UBO + textures)
    // 由 NPROpaquePass 创建，每帧更新 NPRMaterialData
    std::shared_ptr<IDescriptorSetLayout> m_materialDSLayout;
    std::shared_ptr<IDescriptorSet> m_materialDS;
    std::shared_ptr<IBuffer> m_materialUBO;

    // Set 1: SceneData (camera UBO)
    std::shared_ptr<IDescriptorSetLayout> m_sceneDSLayout;
    std::shared_ptr<IDescriptorSet> m_sceneDS;
    std::shared_ptr<IBuffer> m_sceneUBO;

    // Set 3: Lights (SSBO + 计数器 UBO)
    std::shared_ptr<IDescriptorSetLayout> m_lightsDSLayout;
    std::shared_ptr<IDescriptorSet> m_lightsDS;
    std::shared_ptr<IBuffer> m_lightBufferGPU;
    std::shared_ptr<IBuffer> m_lightCountUBO;
};

} // namespace Prisma::Graphic
