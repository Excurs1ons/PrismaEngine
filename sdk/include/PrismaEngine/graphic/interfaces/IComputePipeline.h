#pragma once

#include "RenderTypes.h"
#include "IDescriptorSet.h"
#include "IShader.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class IRenderDevice;

class IComputePipeline {
public:
    virtual ~IComputePipeline() = default;
    [[nodiscard]] virtual PipelineType GetType() const { return PipelineType::Compute; }
    [[nodiscard]] virtual bool IsValid() const = 0;

    virtual void SetShader(std::shared_ptr<IShader> shader) = 0;
    [[nodiscard]] virtual std::shared_ptr<IShader> GetShader() const = 0;

    virtual void SetPushConstantRange(uint32_t size, uint32_t offset = 0) = 0;
    [[nodiscard]] virtual uint32_t GetPushConstantRangeSize() const = 0;
    [[nodiscard]] virtual uint32_t GetPushConstantRangeOffset() const = 0;

    [[nodiscard]] virtual const std::vector<std::shared_ptr<IDescriptorSetLayout>>& GetDescriptorSetLayouts() const = 0;

    virtual bool Create(IRenderDevice* device) = 0;

    virtual void SetDebugName(const std::string& name) = 0;
    virtual const std::string& GetDebugName() const = 0;
};

} // namespace Prisma::Graphic
