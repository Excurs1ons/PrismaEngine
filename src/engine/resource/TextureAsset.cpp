#include "TextureAsset.h"
#include "Logger.h"
#include "../core/ImageLoaderSTB.h"
#include <filesystem>

namespace Prisma {

using namespace Serialization;

bool TextureAsset::Load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("TextureAsset", "纹理文件不存在: {0}", path.string());
        return false;
    }

    SetPath(path);
    SetLoaded(true);
    return true;
}

bool TextureAsset::LoadFromMemory(const uint8_t* data, size_t size) {
    Core::ImageLoaderSTB loader;
    auto result = loader.loadFromMemory(data, size);
    if (!result.success) {
        LOG_ERROR("TextureAsset", "从内存加载纹理失败: {0}", result.error);
        return false;
    }

    m_Width = result.width;
    m_Height = result.height;
    m_Channels = result.channels;
    m_Data = std::move(result.data);

    SetLoaded(true);
    return true;
}

void TextureAsset::Unload() {
    m_Data.clear();
    SetLoaded(false);
}

void TextureAsset::Serialize(OutputArchive& archive) const {
    Asset::Serialize(archive);
    archive.Write("width", m_Width);
    archive.Write("height", m_Height);
    archive.Write("channels", m_Channels);
}

void TextureAsset::Deserialize(InputArchive& archive) {
    Asset::Deserialize(archive);
    archive.Read("width", m_Width);
    archive.Read("height", m_Height);
    archive.Read("channels", m_Channels);

    SetLoaded(true);
}

void TextureAsset::SetDimensions(uint32_t width, uint32_t height, uint32_t channels) {
    m_Width = width;
    m_Height = height;
    m_Channels = channels;
}

void TextureAsset::SetData(const std::vector<uint8_t>& data) {
    m_Data = data;
    SetLoaded(true);
}

} // namespace Prisma
