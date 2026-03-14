#include "Material.h"
#include "Shader.h"
#include "interfaces/ICommandBuffer.h"
#include "Logger.h"

namespace Prisma::Graphic {

// ... 初始化和参数设置保持不变 ...

void Material::Bind(ICommandBuffer* cmd) {
    if (!m_Shader || !cmd) return;

    // 1. 核心逻辑：基于反射自动绑定参数
    const auto& reflection = m_Shader->GetReflection();
    
    // 遍历 Shader 需要的所有资源
    for (const auto& resource : reflection.Resources) {
        // 在材质参数映射表中找同名资源
        auto it = m_Params.find(resource.Name);
        if (it == m_Params.end()) {
            // 记录缺失的参数，方便调试
            LOG_WARN("Material", "Missing required parameter: {0} for shader binding.", resource.Name);
            continue;
        }

        // 2. 根据反射信息进行资源的分发
        const auto& value = it->second;
        
        switch (resource.ResourceType) {
            case ShaderResource::Type::Sampler2D:
            case ShaderResource::Type::SamplerCube:
            case ShaderResource::Type::Image2D: {
                // 如果参数是一个贴图
                if (std::holds_alternative<std::shared_ptr<ITexture>>(value)) {
                    auto texture = std::get<std::shared_ptr<ITexture>>(value);
                    // cmd->BindTexture(resource.Set, resource.Binding, texture.get());
                }
                break;
            }
            case ShaderResource::Type::UniformBuffer: {
                // 如果参数是基础数值 (float, vec3, vec4)
                // 在现代引擎中，这里通常会统一合并到一个共享的 Dynamic Uniform Buffer 里。
                // 我们以后会实现一个 DescriptorManager 来处理这个。
                break;
            }
            default:
                break;
        }
    }

    // 3. 提交底层 Descriptor Set
    // cmd->BindDescriptorSet(1, m_DescriptorSetHandle);
}

} // namespace Prisma::Graphic
