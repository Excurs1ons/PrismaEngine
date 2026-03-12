#include "ResourceFallbackImpl.h"
#include "Logger.h"
#include "Shader.h"
#include "graphic/DefaultShader.h"

namespace PrismaEngine {
namespace Resource {

std::shared_ptr<AssetBase> CreateDefaultMesh(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认三角形网格", relativePath);
    // TODO: 暂时返回nullptr，等待循环依赖问题解决
    return nullptr;
}

std::shared_ptr<AssetBase> CreateDefaultShader(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认着色器", relativePath);

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    LOG_WARNING("ResourceFallback", "Vulkan着色器支持尚未实现，默认着色器创建失败");
    return nullptr;
#else
    LOG_WARNING("ResourceFallback", "无可用渲染后端，默认着色器创建失败");
    return nullptr;
#endif
}

std::shared_ptr<AssetBase> CreateDefaultMaterial(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认材质", relativePath);
    // TODO: 暂时返回nullptr，等待循环依赖问题解决
    return nullptr;
}

} // namespace Resource
} // namespace Engine
