#include "ResourceManager.h"
#include "ResourceFallback.h"
#include "AssetSerializer.h"
#include "MeshAsset.h"
#include "TextureAsset.h"
#include <fstream>

namespace Engine {

ResourceManager::~ResourceManager() {
    {
        UnloadAll();
    }
}

void ResourceManager::CreateDefaultAssets() {
    LOG_INFO("Resource", "开始创建默认资产...");

    // 确保资源管理器已初始化
    if (!IsInitialized()) {
        LOG_ERROR("Resource", "资源管理器尚未初始化，无法创建默认资产");
        return;
    }

    // 确保Assets目录存在
    std::filesystem::path assetsDir    = m_projectRoot / "Assets";
    std::filesystem::path meshesDir    = assetsDir / "Models";
    std::filesystem::path shadersDir   = assetsDir / "Shaders";
    std::filesystem::path texturesDir  = assetsDir / "Textures";
    std::filesystem::path materialsDir = assetsDir / "Materials";

    try {
        std::filesystem::create_directories(meshesDir);
        std::filesystem::create_directories(shadersDir);
        std::filesystem::create_directories(texturesDir);
        std::filesystem::create_directories(materialsDir);
    } catch (const std::exception& e) {
        LOG_ERROR("Resource", "创建资产目录失败: {0}", e.what());
        return;
    }

    // 创建默认网格
    CreateDefaultMeshes(meshesDir);

    // 创建默认着色器
    CreateDefaultShaders(shadersDir);

    // 创建默认纹理
    CreateDefaultTextures(texturesDir);

    // 创建默认材质
    CreateDefaultMaterials(materialsDir);

    LOG_INFO("Resource", "默认资产创建完成");
}

}  // namespace Engine