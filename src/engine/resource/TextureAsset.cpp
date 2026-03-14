#include "TextureAsset.h"
#include "Logger.h"
#include <filesystem>

namespace Prisma {

using namespace Serialization;

bool TextureAsset::Load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("TextureAsset", "Texture file does not exist: {0}", path.string());
        return false;
    }

    SetPath(path);
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
