#include "ResourceFallbackImpl.h"
#include "Logger.h"

namespace Prisma {
namespace Resource {

std::shared_ptr<Asset> CreateDefaultMesh(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default mesh placeholder for: {}", relativePath);
    return nullptr;
}

std::shared_ptr<Asset> CreateDefaultShader(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default shader placeholder for: {}", relativePath);
    return nullptr;
}

std::shared_ptr<Asset> CreateDefaultMaterial(const std::string& relativePath) {
    LOG_INFO("ResourceFallback", "Creating default material placeholder for: {}", relativePath);
    return nullptr;
}

} // namespace Resource
} // namespace Prisma
