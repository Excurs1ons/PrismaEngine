#include "ResourceManagerTypes.h"  // 包含完整类型定义
#include "ResourceFallbackImpl.h"
#include "Logger.h"
#include "graphic/DefaultShader.h"

namespace Engine {
namespace Resource {

std::shared_ptr<ResourceBase> CreateDefaultMesh(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认三角形网格", relativePath);
    // TODO: 暂时返回nullptr，等待循环依赖问题解决
    return nullptr;
}

std::shared_ptr<ResourceBase> CreateDefaultShader(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认着色器", relativePath);

    auto shader = std::make_shared<Shader>();
    if (shader->CompileFromString(Graphic::DEFAULT_VERTEX_SHADER, Graphic::DEFAULT_PIXEL_SHADER)) {
        shader->SetName(std::string("DefaultShader(for ") + relativePath + ")");
        return std::static_pointer_cast<ResourceBase>(shader);
    }

    LOG_ERROR("ResourceFallback", "无法创建默认着色器");
    return nullptr;
}

std::shared_ptr<ResourceBase> CreateDefaultMaterial(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认材质", relativePath);
    // TODO: 暂时返回nullptr，等待循环依赖问题解决
    return nullptr;
}

} // namespace Resource
} // namespace Engine