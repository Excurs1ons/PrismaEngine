#pragma once
#include "ResourceBase.h"
#include <memory>
#include <string>

namespace Engine {
namespace Resource {

// 创建默认资源的具体实现函数
std::shared_ptr<ResourceBase> CreateDefaultMesh(const std::string& relativePath);
std::shared_ptr<ResourceBase> CreateDefaultShader(const std::string& relativePath);
std::shared_ptr<ResourceBase> CreateDefaultMaterial(const std::string& relativePath);

} // namespace Resource
} // namespace Engine