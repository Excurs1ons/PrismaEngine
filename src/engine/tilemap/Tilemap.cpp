#include "Tilemap.h"
#include <glaze/glaze.hpp>
#include <fstream>
#include <sstream>

namespace Prisma::Tilemap {

// ── Glaze meta structs ────────────────────────────────────────────────

struct TileJSON {
    uint32_t tileId = 0;
    uint32_t texIndex = 0;
    uint8_t  collisionFlags = 0;
};

struct LayerJSON {
    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint32_t> tiles;
    bool visible = true;
    int sortingOrder = 0;
};

struct TilemapJSON {
    uint32_t tileSize = 16;
    uint32_t columns = 8;
    std::string texturePath;
    std::vector<TileJSON> tiles;
    std::vector<LayerJSON> layers;
};

} // namespace Prisma::Tilemap

// ── glaze 序列化注册 ──────────────────────────────────────────────

template<>
struct glz::meta<Prisma::Tilemap::TileJSON> {
    using T = Prisma::Tilemap::TileJSON;
    static constexpr auto value = object(
        "tileId",        &T::tileId,
        "texIndex",      &T::texIndex,
        "collisionFlags",&T::collisionFlags
    );
};

template<>
struct glz::meta<Prisma::Tilemap::LayerJSON> {
    using T = Prisma::Tilemap::LayerJSON;
    static constexpr auto value = object(
        "name",         &T::name,
        "width",        &T::width,
        "height",       &T::height,
        "tiles",        &T::tiles,
        "visible",      &T::visible,
        "sortingOrder", &T::sortingOrder
    );
};

template<>
struct glz::meta<Prisma::Tilemap::TilemapJSON> {
    using T = Prisma::Tilemap::TilemapJSON;
    static constexpr auto value = object(
        "tileSize",    &T::tileSize,
        "columns",     &T::columns,
        "texturePath", &T::texturePath,
        "tiles",       &T::tiles,
        "layers",      &T::layers
    );
};

namespace Prisma::Tilemap {

// ── Tilemap ───────────────────────────────────────────────────────

bool Tilemap::LoadFromJSON(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::stringstream ss;
    ss << file.rdbuf();

    TilemapJSON data;
    auto ec = glz::read_json(data, ss.str());
    if (ec) return false;

    // 填充 TileSet
    m_tileSet.SetTileSize(data.tileSize);
    m_tileSet.SetColumns(data.columns);
    m_tileSet.SetTexturePath(data.texturePath);
    for (auto& tj : data.tiles) {
        TileDef def;
        def.tileId       = tj.tileId;
        def.texIndex     = tj.texIndex;
        def.collisionFlags = tj.collisionFlags;
        m_tileSet.AddTileDef(def);
    }

    // 填充 Layers
    m_layers.clear();
    for (auto& lj : data.layers) {
        TileLayer layer;
        layer.name        = lj.name;
        layer.width       = lj.width;
        layer.height      = lj.height;
        layer.tiles       = std::move(lj.tiles);
        layer.visible     = lj.visible;
        layer.sortingOrder = lj.sortingOrder;
        m_layers.push_back(std::move(layer));
    }

    // 从第一层获取地图尺寸
    if (!m_layers.empty()) {
        m_width  = m_layers[0].width;
        m_height = m_layers[0].height;
    }

    return true;
}

bool Tilemap::SaveToJSON(const std::string& path) const {
    TilemapJSON data;
    data.tileSize    = m_tileSet.GetTileSize();
    data.columns     = m_tileSet.GetColumns();
    data.texturePath = m_tileSet.GetTexturePath();

    for (uint32_t i = 0; i < m_tileSet.GetTileCount(); ++i) {
        auto* def = m_tileSet.GetTileDef(i + 1);
        if (def) {
            TileJSON tj;
            tj.tileId         = def->tileId;
            tj.texIndex       = def->texIndex;
            tj.collisionFlags = def->collisionFlags;
            data.tiles.push_back(tj);
        }
    }

    for (auto& l : m_layers) {
        LayerJSON lj;
        lj.name        = l.name;
        lj.width       = l.width;
        lj.height      = l.height;
        lj.tiles       = l.tiles;
        lj.visible     = l.visible;
        lj.sortingOrder = l.sortingOrder;
        data.layers.push_back(std::move(lj));
    }

    std::string json = glz::write_json(data).value_or("{}");
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << json;
    return true;
}

TileLayer* Tilemap::AddLayer(const std::string& name, uint32_t width, uint32_t height) {
    TileLayer layer;
    layer.name   = name;
    layer.width  = width;
    layer.height = height;
    layer.tiles.resize(static_cast<size_t>(width) * height, 0);
    m_layers.push_back(std::move(layer));

    if (m_layers.size() == 1) {
        m_width  = width;
        m_height = height;
    }
    return &m_layers.back();
}

TileLayer* Tilemap::GetLayer(uint32_t index) {
    return (index < m_layers.size()) ? &m_layers[index] : nullptr;
}

const TileLayer* Tilemap::GetLayer(uint32_t index) const {
    return (index < m_layers.size()) ? &m_layers[index] : nullptr;
}

uint32_t Tilemap::GetTile(uint32_t layer, uint32_t x, uint32_t y) const {
    if (layer >= m_layers.size()) return 0;
    return m_layers[layer].GetTile(x, y);
}

void Tilemap::SetTile(uint32_t layer, uint32_t x, uint32_t y, uint32_t tileId) {
    if (layer < m_layers.size())
        m_layers[layer].SetTile(x, y, tileId);
}

bool Tilemap::IsSolid(uint32_t layer, uint32_t x, uint32_t y) const {
    if (layer >= m_layers.size()) return false;
    uint32_t tid = m_layers[layer].GetTile(x, y);
    if (tid == 0) return false;
    auto* def = m_tileSet.GetTileDef(tid);
    return def && (def->collisionFlags & TileCollision::Solid);
}

} // namespace Prisma::Tilemap
