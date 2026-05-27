#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Prisma::Tilemap {

struct TileLayer {
    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint32_t> tiles;
    bool visible = true;
    int sortingOrder = 0;

    uint32_t GetTile(uint32_t x, uint32_t y) const {
        if (x >= width || y >= height) return 0;
        return tiles[y * width + x];
    }

    void SetTile(uint32_t x, uint32_t y, uint32_t tileId) {
        if (x < width && y < height)
            tiles[y * width + x] = tileId;
    }
};

} // namespace Prisma::Tilemap
