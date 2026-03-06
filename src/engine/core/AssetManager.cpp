#include "AssetManager.h"
#include "../resource/AssetSerializer.h"
#include "../resource/MeshAsset.h"
#include "../resource/ResourceFallback.h"
#include "../resource/TextureAsset.h"
#include <fstream>

namespace PrismaEngine {

void AssetManager::CreateDefaultAssets() {
    LOG_INFO("Resource", "开始创建默认资产...");

    if (!IsInitialized()) {
        LOG_ERROR("Resource", "资源管理器尚未初始化，无法创建默认资产");
        return;
    }

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

    CreateDefaultMeshes(meshesDir);
    CreateDefaultShaders(shadersDir);
    CreateDefaultTextures(texturesDir);
    CreateDefaultMaterials(materialsDir);

    LOG_INFO("Resource", "默认资产创建完成");
}

void AssetManager::CreateDefaultMeshes(const std::filesystem::path& meshes_dir) {
    (void)meshes_dir;
    // TODO: 实现默认网格创建
}

void AssetManager::CreateDefaultShaders(const std::filesystem::path& shaders_dir) {
    (void)shaders_dir;
    // TODO: 实现默认着色器创建
}

void AssetManager::CreateDefaultTextures(const std::filesystem::path& textures_dir) {
    (void)textures_dir;
    // TODO: 实现默认纹理创建
}

void AssetManager::CreateDefaultMaterials(const std::filesystem::path& materials_dir) {
    (void)materials_dir;
    // TODO: 实现默认材质创建
}

}  // namespace PrismaEngine
