#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace Prisma::Tilemap {

struct TileDef {
    uint32_t tileId = 0;
    uint32_t texIndex = 0;
    uint8_t  collisionFlags = 0;
    uint8_t  padding[3] = {0};
};

enum TileCollision : uint8_t {
    Solid     = 1 << 0,
    Platform  = 1 << 1,
    Hazard    = 1 << 2,
    Ladder    = 1 << 3,
};

class TileSet {
public:
    uint32_t GetTileSize() const { return m_tileSize; }
    void SetTileSize(uint32_t size) { m_tileSize = size; }

    const std::string& GetTexturePath() const { return m_texturePath; }
    void SetTexturePath(const std::string& path) { m_texturePath = path; }

    uint32_t GetColumns() const { return m_columns; }
    void SetColumns(uint32_t cols) { m_columns = cols; }

    void AddTileDef(const TileDef& def) { m_tiles.push_back(def); }

    const TileDef* GetTileDef(uint32_t id) const {
        if (id == 0) return nullptr;
        for (auto& t : m_tiles)
            if (t.tileId == id) return &t;
        return nullptr;
    }

    uint32_t GetTileCount() const { return static_cast<uint32_t>(m_tiles.size()); }

private:
    uint32_t m_tileSize = 16;
    uint32_t m_columns = 8;
    std::string m_texturePath;
    std::vector<TileDef> m_tiles;
};

} // namespace Prisma::Tilemap
