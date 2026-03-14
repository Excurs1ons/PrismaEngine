#pragma once
#include "Asset.h"
#include <memory>
#include <string>

namespace Prisma {
namespace Resource {

// 创建默认资源的具体实现函数
std::shared_ptr<Asset> CreateDefaultMesh(const std::string& relativePath);
std::shared_ptr<Asset> CreateDefaultShader(const std::string& relativePath);
std::shared_ptr<Asset> CreateDefaultMaterial(const std::string& relativePath);

} // namespace Resource
} // namespace Engine