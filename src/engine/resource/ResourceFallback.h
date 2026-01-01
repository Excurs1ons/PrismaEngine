#pragma once
#include "AssetBase.h"
#include <memory>
#include <string>

// 前向声明
namespace PrismaEngine {
    class Material;
    class Mesh;
    class Shader;
}

namespace PrismaEngine {
namespace Resource {

/// @brief 资源回退工具类 - 用于创建默认资源
class AssetFallback {
public:
    /// @brief 为指定资源路径创建默认资源
    static std::shared_ptr<AssetBase> CreateDefaultResource(
        AssetType type,
        const std::string& relativePath
    );

    /// @brief 为加载失败的资源创建回退资源
    static std::shared_ptr<AssetBase> CreateFallbackResource(
        AssetType type,
        const std::string& relativePath,
        std::shared_ptr<AssetBase> failedResource
    );

};

} // namespace Resource
} // namespace Engine