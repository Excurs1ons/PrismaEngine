#pragma once

#include "../forward/ForwardRenderPassBase.h"
#include "graphic/Renderer.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace Prisma::Graphic {

class ICommandBuffer;
class IShader;
class IPipelineState;
class IBuffer;
class IDescriptorSet;
class IDescriptorSetLayout;

/**
 * @brief Clustered Opaque Pass
 */
class ClusteredOpaquePass : public ForwardRenderPass {
public:
    ClusteredOpaquePass();
    ~ClusteredOpaquePass() override = default;

    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    void Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands, IRenderDevice* device);

    void SetClusteredResources(IBuffer* lightBuffer, IBuffer* indexList, IBuffer* grid, IBuffer* cameraUBO);

private:
    bool EnsurePipeline(IRenderDevice* device);

    IBuffer* m_lightBuffer = nullptr;
    IBuffer* m_globalLightIndexList = nullptr;
    IBuffer* m_clusterGrid = nullptr;
    IBuffer* m_cameraUBO = nullptr;

    std::shared_ptr<IBuffer> m_sceneUBO;
    std::shared_ptr<IBuffer> m_objectUBO;

    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_pixelShader;
    std::shared_ptr<IPipelineState> m_pso;
    
    std::shared_ptr<IDescriptorSet> m_clusteredDS;
    std::shared_ptr<IDescriptorSetLayout> m_clusteredDSLayout;

    std::shared_ptr<IDescriptorSet> m_sceneDS;
    std::shared_ptr<IDescriptorSetLayout> m_sceneDSLayout;
};

} // namespace Prisma::Graphic
