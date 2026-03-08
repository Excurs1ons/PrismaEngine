#include "Asset.h"
#include "AssetSerializer.h"
#include <iostream>

namespace PrismaEngine {

bool Asset::SerializeToFile(const std::filesystem::path& filePath, SerializationFormat format) const {
    return AssetSerializer::SerializeToFile(*this, filePath, format);
}

bool Asset::DeserializeFromFile(const std::filesystem::path& filePath, SerializationFormat format) {
    // 默认实现，子类应重写
    return false;
}

} // namespace PrismaEngine
