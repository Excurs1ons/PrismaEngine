#pragma once
#include "AssetBase.h"
#include <memory>
#include <string>

namespace PrismaEngine {
namespace Resource {

// 创建默认资源的具体实现函数
std::shared_ptr<AssetBase> CreateDefaultMesh(const std::string& relativePath);
std::shared_ptr<AssetBase> CreateDefaultShader(const std::string& relativePath);
std::shared_ptr<AssetBase> CreateDefaultMaterial(const std::string& relativePath);

} // namespace Resource
} // namespace Engine