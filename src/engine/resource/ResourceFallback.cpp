#include "ResourceFallback.h"
#include "Logger.h"
#include "ResourceFallbackImpl.h"

namespace Engine {
namespace Resource {

std::shared_ptr<ResourceBase> ResourceFallback::CreateDefaultResource(
    ResourceType type,
    const std::string& relativePath)
{
    switch (type) {
        case ResourceType::Shader:
            return CreateDefaultShader(relativePath);

        case ResourceType::Mesh:
            return CreateDefaultMesh(relativePath);

        case ResourceType::Material:
            return CreateDefaultMaterial(relativePath);

        default:
            LOG_ERROR("ResourceFallback", "不支持的资源类型 {0} 创建默认资源", static_cast<int>(type));
            return nullptr;
    }
}

std::shared_ptr<ResourceBase> ResourceFallback::CreateFallbackResource(
    ResourceType type,
    const std::string& relativePath,
    std::shared_ptr<ResourceBase> failedResource)
{
    LOG_WARNING("ResourceFallback", "资源 {0} 加载失败，创建默认回退资源", relativePath);

    auto fallback = CreateDefaultResource(type, relativePath);
    if (fallback) {
        fallback->SetName(std::string("DefaultResource(fallback from ") + relativePath + ")");
    }

    return fallback;
}

} // namespace Resource
} // namespace Engine