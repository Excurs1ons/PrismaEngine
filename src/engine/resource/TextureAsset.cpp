#include "TextureAsset.h"
#include "AssetSerializer.h"
#include "Logger.h"
#include <filesystem>

namespace Prisma {

using namespace Serialization;

bool TextureAsset::Load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("TextureAsset", "Texture file does not exist: {0}", path.string());
        return false;
    }

    Path = path;
    Name = path.filename().string();
    m_Metadata.sourcePath = path;
    m_Metadata.name = Name;
    SetLoaded(true);
    return true;
}

void TextureAsset::Unload() {
    Asset::Unload();
    m_Data.clear();
}

void TextureAsset::Serialize(OutputArchive& archive) const {
    Asset::Serialize(archive);
    archive("width", m_Width);
    archive("height", m_Height);
    archive("channels", m_Channels);
}

void TextureAsset::Deserialize(InputArchive& archive) {
    Asset::Deserialize(archive);
    archive("width", m_Width);
    archive("height", m_Height);
    archive("channels", m_Channels);

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
