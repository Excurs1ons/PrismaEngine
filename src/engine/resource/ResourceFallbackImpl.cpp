#include "ResourceFallbackImpl.h"
#include "Logger.h"

namespace PrismaEngine {
namespace Resource {

std::shared_ptr<AssetBase> CreateDefaultMesh(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default mesh placeholder for: {}", relativePath);
    return nullptr;
}

std::shared_ptr<AssetBase> CreateDefaultShader(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default shader placeholder for: {}", relativePath);
    return nullptr;
}

std::shared_ptr<AssetBase> CreateDefaultMaterial(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default material placeholder for: {}", relativePath);
    return nullptr;
}

} // namespace Resource
} // namespace PrismaEngine
