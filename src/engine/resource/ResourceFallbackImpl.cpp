#include "ResourceFallbackImpl.h"
#include "Logger.h"
#include "graphic/Mesh.h"
#include "graphic/Shader.h"
#include "graphic/Material.h"
#include "graphic/DefaultShader.h"

namespace Engine {
namespace Resource {

std::shared_ptr<ResourceBase> CreateDefaultMesh(const std::string& relativePath)
{
    LOG_INFO("ResourceFallback", "为 {0} 创建默认三角形网格", relativePath);

    auto mesh = std::make_shared<Mesh>();
    *mesh = Mesh::GetTriangleMesh();
    mesh->SetName(std::string("DefaultTriangleMesh(for ") + relativePath + ")");
    return std::static_pointer_cast<ResourceBase>(mesh);
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

    auto material = Material::CreateDefault();
    if (material) {
        material->SetName(std::string("DefaultMaterial(for ") + relativePath + ")");
        return std::static_pointer_cast<ResourceBase>(material);
    }
    return nullptr;
}

} // namespace Resource
} // namespace Engine