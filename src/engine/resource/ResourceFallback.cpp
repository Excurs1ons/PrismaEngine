#include "ResourceFallback.h"
#include "Logger.h"
#include "graphic/DefaultShader.h"
#include "graphic/Material.h"
#include "graphic/Mesh.h"

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
        fallback->m_name = std::string("DefaultResource(fallback from ") + relativePath + ")";
    }

    return fallback;
}

std::shared_ptr<Shader> ResourceFallback::CreateDefaultShader(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认着色器", relativePath);

    auto shader = std::make_shared<Shader>();
    if (shader->CompileFromString(Graphic::DEFAULT_VERTEX_SHADER, Graphic::DEFAULT_PIXEL_SHADER)) {
        shader->m_name = std::string("DefaultShader(for ") + relativePath + ")";
        shader->m_isLoaded = true;
        return shader;
    }

    LOG_ERROR("ResourceFallback", "无法创建默认着色器");
    return nullptr;
}

std::shared_ptr<Mesh> ResourceFallback::CreateDefaultMesh(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认三角形网格", relativePath);

    auto mesh = std::make_shared<Mesh>();
    *mesh = Mesh::GetTriangleMesh();
    mesh->m_name = std::string("DefaultTriangleMesh(for ") + relativePath + ")";
    mesh->m_isLoaded = true;
    return mesh;
}

std::shared_ptr<Material> ResourceFallback::CreateDefaultMaterial(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认材质", relativePath);

    auto material = Material::CreateDefault();
    if (material) {
        material->m_name = std::string("DefaultMaterial(for ") + relativePath + ")";
    }
    return material;
}

} // namespace Resource
} // namespace Engine