#pragma once
#include "ResourceBase.h"
#include <memory>
#include <string>

// 前向声明
namespace Engine {
    class Material;
    class Mesh;
    class Shader;
}

namespace Engine {
namespace Resource {

/// @brief 资源回退工具类 - 用于创建默认资源
class ResourceFallback {
public:
    /// @brief 为指定资源路径创建默认资源
    static std::shared_ptr<ResourceBase> CreateDefaultResource(
        ResourceType type,
        const std::string& relativePath
    );

    /// @brief 为加载失败的资源创建回退资源
    static std::shared_ptr<ResourceBase> CreateFallbackResource(
        ResourceType type,
        const std::string& relativePath,
        std::shared_ptr<ResourceBase> failedResource
    );

};

} // namespace Resource
} // namespace Engine