#include "TilemapAsset.h"
//#include "core/MapParser.h"
#include "Logger.h"

namespace Prisma {

bool TilemapAsset::Load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("TilemapAsset", "Tilemap file does not exist: {0}", path.string());
        return false;
    }

    // m_map = TileMapParser::ParseFile(path.string());
    
    SetPath(path);
    SetName(path.stem().string());
    m_isLoaded = true;
    return true;
}

void TilemapAsset::Unload() {
    m_map.reset();
    m_isLoaded = false;
}

void TilemapAsset::Serialize(Serialization::OutputArchive& archive) const {
    Asset::Serialize(archive);
    // Basic serialization
    archive.Write("layerCount", static_cast<uint32_t>(GetLayerCount()));
}

void TilemapAsset::Deserialize(Serialization::InputArchive& archive) {
    Asset::Deserialize(archive);
    // Basic deserialization
}

} // namespace Prisma
