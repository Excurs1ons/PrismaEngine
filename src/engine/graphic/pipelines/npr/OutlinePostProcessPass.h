#pragma once

#include "interfaces/RenderTypes.h"
#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/IRenderTarget.h"
#include <memory>
#include "Export.h"

namespace Prisma::Graphic {

// NPR 轮廓线后处理 Pass
/// 使用 Sobel 算子在颜色纹理上检测边缘，叠加黑色轮廓线
class ENGINE_API OutlinePostProcessPass {
public:
    OutlinePostProcessPass();
    ~OutlinePostProcessPass();

    bool Setup(IRenderDevice* device);
    void Execute(ICommandBuffer* cmd, ITexture* colorInput, ITextureRenderTarget* output);
    void Cleanup();
    bool IsReady() const { return m_ready; }

    void SetOutlineColor(const Color& color) { m_outlineColor = color; }
    void SetOutlineWidth(float width) { m_outlineWidth = width; }

private:
    bool CreateTextures(uint32_t width, uint32_t height);
    bool CreatePipelines();
    bool CreateDescriptorSets(uint32_t width, uint32_t height);

    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;

    Color m_outlineColor{0.0f, 0.0f, 0.0f, 1.0f};
    float m_outlineWidth = 1.0f;

    std::shared_ptr<IShader> m_edgeShader;
    std::shared_ptr<IShader> m_compositeShader;

    std::shared_ptr<IComputePipeline> m_edgePipeline;
    std::shared_ptr<IComputePipeline> m_compositePipeline;

    std::shared_ptr<ITexture> m_edgeTexture;

    std::shared_ptr<IDescriptorSetLayout> m_edgeDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_edgeDescSet;
    std::shared_ptr<IDescriptorSetLayout> m_compositeDescSetLayout;
    std::shared_ptr<IDescriptorSet> m_compositeDescSet;

    std::shared_ptr<ISampler> m_linearSampler;

    // Push constant layout: { outlineColor (4xf32), outlineWidth (f32) }
    struct alignas(16) EdgePushConstants {
        float color[4];
        float width;
    };

    bool m_ready = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
