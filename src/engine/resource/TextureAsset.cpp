#include "TextureAsset.h"
#include "AssetSerializer.h"
#include "Logger.h"
#include <filesystem>

namespace PrismaEngine {

using namespace Serialization;

bool TextureAsset::Load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("Texture", "Texture file does not exist: {0}", path.string());
        return false;
    }

    m_path = path;
    m_name = path.filename().string();
    m_metadata.sourcePath = path;
    m_metadata.name = m_name;
    m_isLoaded = true;
    return true;
}

void TextureAsset::Unload() {
    m_isLoaded = false;
}

void TextureAsset::Serialize(OutputArchive& archive) const {
    archive.BeginObject("TextureAsset");
    archive("metadata", m_metadata);
    archive("width", width);
    archive("height", height);
    archive("channels", channels);
    archive.EndObject();
}

void TextureAsset::Deserialize(InputArchive& archive) {
    archive.BeginObject("TextureAsset");
    archive("metadata", m_metadata);
    archive("width", width);
    archive("height", height);
    archive("channels", channels);
    archive.EndObject();

    m_isLoaded = true;
    m_name = m_metadata.name;
}

bool TextureAsset::DeserializeFromFile(const std::filesystem::path& path, SerializationFormat format) {
    auto deserializedAsset = AssetSerializer::DeserializeFromFile<TextureAsset>(path, format);
    if (deserializedAsset) {
        m_metadata = deserializedAsset->m_metadata;
        width = deserializedAsset->width;
        height = deserializedAsset->height;
        channels = deserializedAsset->channels;
        m_path = path;
        m_name = deserializedAsset->m_name;
        m_isLoaded = true;
        return true;
    }
    return false;
}

} // namespace PrismaEngine
