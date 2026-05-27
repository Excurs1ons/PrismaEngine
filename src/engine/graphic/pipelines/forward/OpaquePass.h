#pragma once

#include "interfaces/RenderTypes.h"
#include "graphic/Renderer.h"
#include "ForwardRenderPassBase.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ICommandBuffer;
class IShader;
class IPipelineState;

/* 不透明渲染通道 (Opaque Pass) */
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

private:
    bool EnsureDefaultPipeline();

    std::vector<Light> m_Lights;
    IRenderDevice* m_device = nullptr;
    std::unordered_map<uint64_t, std::shared_ptr<IPipelineState>> m_psoCache;
    std::shared_ptr<IShader> m_defaultVertexShader;
    std::shared_ptr<IShader> m_defaultPixelShader;
    std::shared_ptr<IPipelineState> m_defaultPipelineState;
};

} // namespace Prisma::Graphic
