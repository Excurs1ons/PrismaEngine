#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace PrismaEngine {

// ============================================================================
// 前向声明
// ============================================================================

class Layer;

// ============================================================================
// 图块层
// ============================================================================

struct TileLayerData {
    int width = 0;
    int height = 0;
    std::vector<uint32_t> data;  // GID 数组

    // 无限地图块数据
    struct Chunk {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        std::vector<uint32_t> data;
    };
    std::vector<Chunk> chunks;

    // 检查是否为无限地图
    bool IsInfinite() const {
        return !chunks.empty();
    }

    // 获取指定位置的 GID
    uint32_t GetGid(int x, int y) const {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return 0;
        }
        size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
        if (index >= data.size()) {
            return 0;
        }
        return data[index];
    }

    // 设置指定位置的 GID
    void SetGid(int x, int y, uint32_t gid) {
        if (x >= 0 && y >= 0 && x < width && y < height) {
            size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
            if (index < data.size()) {
                data[index] = gid;
            }
        }
    }
};

class TileLayer {
public:
    TileLayer() = default;
    virtual ~TileLayer() = default;

    // 基本信息
    std::string name;
    int id = 0;
    bool visible = true;
    float opacity = 1.0f;
    int offsetX = 0;
    int offsetY = 0;
    float parallaxX = 1.0f;
    float parallaxY = 1.0f;
    std::string tint;              // 十六进制颜色 (#RRGGBB 或 #AARRGGBB)

    // 瓦片数据
    TileLayerData tileData;

    // 自定义属性
    PropertyMap properties;

    // 类型标识
    static constexpr LayerType Type = LayerType::TileLayer;
    LayerType GetLayerType() const { return Type; }
};

// ============================================================================
// 对象层
// ============================================================================

// 对象基类
struct MapObject {
    int id = 0;
    std::string name;
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float rotation = 0.0f;         // 旋转角度 (度)
    uint32_t gid = 0;              // 瓦片 GID (如果是瓦片对象)
    bool visible = true;

    // 多边形/折线点
    std::vector<std::pair<float, float>> points;

    // 文本对象
    TextObject* text = nullptr;

    // 自定义属性
    PropertyMap properties;

    // 类型
    ObjectType objectType = ObjectType::Rectangle;

    // 虚析构以支持多态
    virtual ~MapObject() {
        delete text;
    }

    // 检查是否为瓦片对象
    bool IsTileObject() const {
        return objectType == ObjectType::Tile && gid != 0;
    }

    // 检查是否有旋转
    bool HasRotation() const {
        return rotation != 0.0f;
    }

    // 获取中心点
    std::pair<float, float> GetCenter() const {
        return {x + width / 2.0f, y + height / 2.0f};
    }
};

class ObjectLayer {
public:
    ObjectLayer() = default;
    ~ObjectLayer() = default;

    // 基本信息
    std::string name;
    int id = 0;
    bool visible = true;
    float opacity = 1.0f;
    int offsetX = 0;
    int offsetY = 0;
    float parallaxX = 1.0f;
    float parallaxY = 1.0f;
    std::string tint;

    // 对象列表
    std::vector<std::unique_ptr<MapObject>> objects;

    // 绘制顺序
    DrawOrder drawOrder = DrawOrder::Index;

    // 自定义属性
    PropertyMap properties;

    // 类型标识
    static constexpr LayerType Type = LayerType::ObjectLayer;
    LayerType GetLayerType() const { return Type; }

    // 查找对象
    MapObject* FindObject(int objectId) const {
        for (const auto& obj : objects) {
            if (obj->id == objectId) {
                return obj.get();
            }
        }
        return nullptr;
    }

    MapObject* FindObjectByName(const std::string& objectName) const {
        for (const auto& obj : objects) {
            if (obj->name == objectName) {
                return obj.get();
            }
        }
        return nullptr;
    }
};

// ============================================================================
// 图像层
// ============================================================================

class ImageLayer {
public:
    ImageLayer() = default;
    ~ImageLayer() = default;

    // 基本信息
    std::string name;
    int id = 0;
    bool visible = true;
    float opacity = 1.0f;
    int offsetX = 0;
    int offsetY = 0;
    float parallaxX = 1.0f;
    float parallaxY = 1.0f;
    std::string tint;

    // 图像信息
    std::string imagePath;
    int imageWidth = 0;
    int imageHeight = 0;

    // 自定义属性
    PropertyMap properties;

    // 类型标识
    static constexpr LayerType Type = LayerType::ImageLayer;
    LayerType GetLayerType() const { return Type; }
};

// ============================================================================
// 组层
// ============================================================================

class GroupLayer {
public:
    GroupLayer() = default;
    ~GroupLayer() = default;

    // 基本信息
    std::string name;
    int id = 0;
    bool visible = true;
    float opacity = 1.0f;
    int offsetX = 0;
    int offsetY = 0;
    float parallaxX = 1.0f;
    float parallaxY = 1.0f;
    std::string tint;

    // 自定义属性
    PropertyMap properties;

    // 子层列表
    std::vector<std::unique_ptr<Layer>> layers;

    // 类型标识
    static constexpr LayerType Type = LayerType::GroupLayer;
    LayerType GetLayerType() const { return Type; }

    // 查找子层
    Layer* FindLayer(int layerId) const;
    Layer* FindLayerByName(const std::string& layerName) const;
};

} // namespace PrismaEngine
