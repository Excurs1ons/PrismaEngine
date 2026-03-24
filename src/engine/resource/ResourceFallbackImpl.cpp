#include "ResourceFallbackImpl.h"
#include "Logger.h"
#include "../graphic/Material.h"
#include "../graphic/Shader.h"
#include "MeshAsset.h"
#include <filesystem>

namespace Prisma {
namespace Resource {

std::shared_ptr<Asset> CreateDefaultMesh(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default mesh placeholder for: {}", relativePath);

    auto mesh = std::make_shared<MeshAsset>();
    mesh->SetName(std::filesystem::path(relativePath).stem().string().empty() ? "DefaultMesh" : std::filesystem::path(relativePath).stem().string());
    mesh->SetPath(relativePath);

    SubMesh triangle;
    triangle.name = "FallbackTriangle";
    triangle.materialIndex = 0;
    triangle.vertices.resize(3);
    triangle.vertices[0].position = {-0.5f, -0.5f, 0.0f, 1.0f};
    triangle.vertices[1].position = {0.0f, 0.5f, 0.0f, 1.0f};
    triangle.vertices[2].position = {0.5f, -0.5f, 0.0f, 1.0f};
    triangle.indices = {0, 1, 2};

    mesh->AddSubMesh(triangle);
    mesh->SetBoundingBox(Graphic::BoundingBox{{-0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}});
    mesh->SetLoaded(true);
    return mesh;
}

std::shared_ptr<Asset> CreateDefaultShader(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default shader placeholder for: {}", relativePath);

    auto shader = std::make_shared<Graphic::Shader>();
    shader->SetName(std::filesystem::path(relativePath).stem().string().empty() ? "DefaultShader" : std::filesystem::path(relativePath).stem().string());
    shader->SetPath(relativePath);
    shader->Load(relativePath);
    return shader;
}

std::shared_ptr<Asset> CreateDefaultMaterial(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default material placeholder for: {}", relativePath);

    auto shader = std::dynamic_pointer_cast<Graphic::Shader>(CreateDefaultShader(relativePath));
    auto material = std::make_shared<Graphic::Material>(shader);
    material->SetName(std::filesystem::path(relativePath).stem().string().empty() ? "DefaultMaterial" : std::filesystem::path(relativePath).stem().string());
    material->SetPath(relativePath);
    material->SetBaseColor(1.0f, 0.2f, 1.0f, 1.0f);
    material->SetMetallic(0.0f);
    material->SetRoughness(1.0f);
    material->SetLoaded(true);
    return material;
}

} // namespace Resource
} // namespace Prisma
