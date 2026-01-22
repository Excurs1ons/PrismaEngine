#pragma once

#include "Types.h"
#include "Tile.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace PrismaEngine {

// ============================================================================
// 图块变换
// ============================================================================

struct Transformation {
    bool hflip = false;   // 水平翻转
    bool vflip = false;   // 垂直翻转
    bool rotate = false;  // 旋转
};

// ============================================================================
// Wang 标记 (用于自动拼接地形)
// ============================================================================

struct WangTile {
    int tileId = 0;       // 瓦片 ID
    uint8_t wangId = 0;   // Wang ID
};

struct WangColor {
    std::string name;
    std::string color;
    int tile = -1;
    float probability = 1.0f;
};

struct WangSet {
    std::string name;
    int tile = -1;        // 代表此 WangSet 的瓦片
    // Corner colors
    std::vector<WangColor> cornerColors;
    // Edge colors
    std::vector<WangColor> edgeColors;
    // Wang tiles
    std::vector<WangTile> wangTiles;
};

// ============================================================================
// 图块集
// ============================================================================

class Tileset {
public:
    Tileset() = default;
    ~Tileset() = default;

    // 基本信息
    std::string name;              // 图块集名称
    int firstGid = 0;              // 第一个全局瓦片 ID
    int tileWidth = 0;             // 瓦片宽度 (像素)
    int tileHeight = 0;            // 瓦片高度 (像素)
    int spacing = 0;               // 瓦片间距
    int margin = 0;                // 边距
    int tileCount = 0;             // 瓦片总数
    int columns = 0;               // 列数
    int objectAlignment = 0;       // 对齐方式

    // 图像信息 (基于图像的图块集)
    std::string imagePath;         // 图像路径 (相对于 TMX)
    int imageWidth = 0;            // 图像宽度
    int imageHeight = 0;           // 图像高度

    // Image Collection 模式
    struct ImageTile {
        int id = -1;
        std::string imagePath;
        int imageWidth = 0;
        int imageHeight = 0;
    };
    std::vector<ImageTile> images;

    // 颜色键 (透明色)
    struct Color {
        int r = 0, g = 0, b = 0;
    } transparentColor;

    // 外部 TSX 引用
    std::string source;            // 外部 TSX 文件路径

    // 瓦片偏移
    TileOffset tileOffset;

    // 网格 (仅用于六边形地图)
    struct Grid {
        Orientation orientation = Orientation::Orthogonal;
        int width = 0;
        int height = 0;
    } grid;

    // 自定义属性
    PropertyMap properties;

    // 特殊瓦片 (id -> Tile)
    std::unordered_map<int, Tile> tiles;

    // Wang sets (用于自动地形)
    std::vector<WangSet> wangSets;

    // 变换
    std::vector<Transformation> transformations;

    // =========================================================================
    // 工具方法
    // =========================================================================

    // 根据 GID 查找瓦片
    const Tile* FindTile(int localId) const {
        auto it = tiles.find(localId);
        if (it != tiles.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // 检查是否为 Image Collection 模式
    bool IsImageCollection() const {
        return !images.empty();
    }

    // 检查是否为外部引用
    bool IsExternal() const {
        return !source.empty();
    }

    // 获取指定瓦片的图像路径
    std::string GetTileImagePath(int localId) const {
        if (IsImageCollection()) {
            for (const auto& img : images) {
                if (img.id == localId) {
                    return img.imagePath;
                }
            }
        }
        return imagePath;
    }

    // 获取指定瓦片在图像中的 UV 坐标
    void GetTileUV(int localId, float& u0, float& v0, float& u1, float& v1) const {
        if (columns <= 0 || imageWidth <= 0 || imageHeight <= 0) {
            u0 = v0 = u1 = v1 = 0.0f;
            return;
        }

        int index = localId; // localId 是相对于 firstGid 的
        int row = index / columns;
        int col = index % columns;

        float tileU = float(tileWidth) / float(imageWidth);
        float tileV = float(tileHeight) / float(imageHeight);
        float startX = float(col * (tileWidth + spacing) + margin) / float(imageWidth);
        float startY = float(row * (tileHeight + spacing) + margin) / float(imageHeight);

        u0 = startX;
        v0 = startY;
        u1 = startX + tileU;
        v1 = startY + tileV;
    }

    // 获取最大全局 GID
    int GetMaxGid() const {
        return firstGid + tileCount - 1;
    }

    // 检查 GID 是否属于此图块集
    bool ContainsGid(uint32_t gid) const {
        uint32_t pureGid = GIDHelper::GetPureGid(gid);
        return pureGid >= static_cast<uint32_t>(firstGid) &&
               pureGid <= static_cast<uint32_t>(GetMaxGid());
    }
};

} // namespace PrismaEngine
