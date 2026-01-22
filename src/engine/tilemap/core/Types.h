#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace PrismaEngine {

// ============================================================================
// 地图方向枚举
// ============================================================================

enum class Orientation {
    Orthogonal,   // 正交 (标准 2D)
    Isometric,    // 等距
    Staggered,    // 交错
    Hexagonal     // 六边形
};

enum class RenderOrder {
    RightDown,    // 从右到下 (默认)
    RightUp,      // 从右到上
    LeftDown,     // 从左到下
    LeftUp        // 从左到上
};

enum class StaggerIndex {
    Odd,          // 奇数行/列
    Even          // 偶数行/列
};

enum class StaggerAxis {
    X,            // X 轴交错
    Y             // Y 轴交错
};

enum class HexSideLength {
    SideLength    // 六边形边长
};

// ============================================================================
// 层类型枚举
// ============================================================================

enum class LayerType {
    TileLayer,    // 瓦片层
    ObjectLayer,  // 对象层
    ImageLayer,   // 图像层
    GroupLayer    // 组层
};

// ============================================================================
// 对象类型枚举
// ============================================================================

enum class ObjectType {
    Rectangle,    // 矩形
    Ellipse,      // 椭圆
    Point,        // 点
    Polygon,      // 多边形
    Polyline,     // 折线
    Text,         // 文本
    Tile          // 瓦片对象
};

// ============================================================================
// 瓦片翻转标志 (GID 高位)
// ============================================================================

enum class TileFlip : uint32_t {
    None         = 0,
    Horizontal   = 0x80000000,  // 水平翻转
    Vertical     = 0x40000000,  // 垂直翻转
    Diagonal     = 0x20000000,  // 对角翻转 (旋转)
    All          = Horizontal | Vertical | Diagonal,
    Mask         = 0xE0000000   // 翻转掩码
};

inline TileFlip operator|(TileFlip a, TileFlip b) {
    return static_cast<TileFlip>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline TileFlip operator&(TileFlip a, TileFlip b) {
    return static_cast<TileFlip>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// ============================================================================
// GID (全局瓦片 ID) 工具函数
// ============================================================================

namespace GIDHelper {
    // 提取纯 GID (去除翻转标志)
    inline uint32_t GetPureGid(uint32_t gid) {
        return gid & ~static_cast<uint32_t>(TileFlip::Mask);
    }

    // 检查是否有翻转标志
    inline bool HasFlip(uint32_t gid) {
        return (gid & static_cast<uint32_t>(TileFlip::Mask)) != 0;
    }

    // 检查水平翻转
    inline bool IsHorizontallyFlipped(uint32_t gid) {
        return (gid & static_cast<uint32_t>(TileFlip::Horizontal)) != 0;
    }

    // 检查垂直翻转
    inline bool IsVerticallyFlipped(uint32_t gid) {
        return (gid & static_cast<uint32_t>(TileFlip::Vertical)) != 0;
    }

    // 检查对角翻转
    inline bool IsDiagonallyFlipped(uint32_t gid) {
        return (gid & static_cast<uint32_t>(TileFlip::Diagonal)) != 0;
    }

    // 清除翻转标志
    inline uint32_t ClearFlip(uint32_t gid) {
        return GetPureGid(gid);
    }

    // 设置翻转标志
    inline uint32_t SetFlip(uint32_t gid, TileFlip flip) {
        return GetPureGid(gid) | static_cast<uint32_t>(flip);
    }
}

// ============================================================================
// 数据编码/压缩格式
// ============================================================================

enum class TileDataEncoding {
    CSV,          // 逗号分隔值
    Base64,       // Base64 编码
    Base64_Zlib,  // Base64 + zlib 压缩
    Base64_Zstd,  // Base64 + zstd 压缩
    Base64_Gzip   // Base64 + gzip 压缩
};

// ============================================================================
// 绘制顺序
// ============================================================================

enum class DrawOrder {
    Index,        // 按索引顺序 (默认)
    Topdown       // 从上到下 (用于 Y-sort)
};

// ============================================================================
// 地形类型
// ============================================================================

struct Terrain {
    std::string name;
    int tile = -1;  // 代表此地形的瓦片
};

// ============================================================================
// 自定义属性
// ============================================================================

enum class PropertyType {
    String,
    Int,
    Float,
    Bool,
    Color,
    File,
    Object,
    Class
};

struct Property {
    PropertyType type = PropertyType::String;
    std::string name;
    std::string value;  // 存储为字符串，按需转换

    // 便捷访问方法
    std::string AsString() const { return value; }
    int AsInt() const { return std::stoi(value); }
    float AsFloat() const { return std::stof(value); }
    bool AsBool() const { return value == "true"; }
};

using PropertyMap = std::unordered_map<std::string, Property>;

// ============================================================================
// 文本对象
// ============================================================================

struct TextObject {
    std::string text;
    std::string fontFamily = "sans-serif";
    int pixelSize = 16;
    bool wrap = false;
    std::string color = "#000000";
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikeout = false;
    int kerning = 0;
    bool hAlign = false;  // false=left, true=center
    bool vAlign = false;  // false=top, true=center
};

// ============================================================================
// 边距和间距
// ============================================================================

struct Margin {
    int top = 0;
    int left = 0;
    int right = 0;
    int bottom = 0;
};

struct TileOffset {
    int x = 0;
    int y = 0;
};

} // namespace PrismaEngine
