#pragma once

#include "TileSet.h"
#include "TileLayer.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma::Tilemap {

class Tilemap {
public:
    Tilemap() = default;

    bool LoadFromJSON(const std::string& path);
    bool SaveToJSON(const std::string& path) const;

    TileSet& GetTileSet() { return m_tileSet; }
    const TileSet& GetTileSet() const { return m_tileSet; }

    TileLayer* AddLayer(const std::string& name, uint32_t width, uint32_t height);
    TileLayer* GetLayer(uint32_t index);
    const TileLayer* GetLayer(uint32_t index) const;
    uint32_t GetLayerCount() const { return static_cast<uint32_t>(m_layers.size()); }

    uint32_t GetTile(uint32_t layer, uint32_t x, uint32_t y) const;
    void SetTile(uint32_t layer, uint32_t x, uint32_t y, uint32_t tileId);

    bool IsSolid(uint32_t layer, uint32_t x, uint32_t y) const;

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetTileSize() const { return m_tileSet.GetTileSize(); }

private:
    TileSet m_tileSet;
    std::vector<TileLayer> m_layers;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Tilemap
