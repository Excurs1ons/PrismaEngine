#pragma once

#ifdef PRISMA_ENABLE_RENDER_VULKAN
#ifndef MY_APPLICATION_CUBEMAPTEXTUREASSET_H
#define MY_APPLICATION_CUBEMAPTEXTUREASSET_H

#include "TextureAsset.h"
#include <memory>
#include <string>
#include <vector>

class VulkanContext;

class CubemapTextureAsset : public TextureAsset {
public:
    static std::shared_ptr<CubemapTextureAsset> loadCubemap(
            const std::vector<std::string>& facePaths,
            VulkanContext* vulkanContext);

    CubemapTextureAsset();
    ~CubemapTextureAsset();

private:
    bool loadFromFiles(const std::vector<std::string>& facePaths, VulkanContext* context);
};

#endif
#endif
