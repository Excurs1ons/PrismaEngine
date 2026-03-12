#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include "CubemapTextureAsset.h"

CubemapTextureAsset::CubemapTextureAsset() : TextureAsset() {}

CubemapTextureAsset::~CubemapTextureAsset() {
}

std::shared_ptr<CubemapTextureAsset> CubemapTextureAsset::loadCubemap(
        const std::vector<std::string>& facePaths,
        VulkanContext* vulkanContext) {
    return loadFromFiles(facePaths, vulkanContext) 
        ? std::shared_ptr<CubemapTextureAsset>(new CubemapTextureAsset())
        : nullptr;
}

bool CubemapTextureAsset::loadFromFiles(const std::vector<std::string>& facePaths, VulkanContext* context) {
    return false;
}
#endif // PRISMA_ENABLE_RENDER_VULKAN
