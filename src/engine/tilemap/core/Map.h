#pragma once

#include "Types.h"
#include "Tileset.h"
#include "TileLayer.h"
#include "ObjectLayer.h"
#include "ImageLayer.h"
#include "GroupLayer.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace PrismaEngine {

// ============================================================================
// 通用层基类 (多态包装)
// ============================================================================

class Layer {
public:
    Layer() = default;
    virtual ~Layer() = default;

    // 类型标识
    virtual LayerType GetType() const = 0;

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

    // 转换方法
    TileLayer* AsTileLayer() {
        return GetType() == LayerType::TileLayer ? static_cast<TileLayer*>(this) : nullptr;
    }
    ObjectLayer* AsObjectLayer() {
        return GetType() == LayerType::ObjectLayer ? static_cast<ObjectLayer*>(this) : nullptr;
    }
    ImageLayer* AsImageLayer() {
        return GetType() == LayerType::ImageLayer ? static_cast<ImageLayer*>(this) : nullptr;
    }
    GroupLayer* AsGroupLayer() {
        return GetType() == LayerType::GroupLayer ? static_cast<GroupLayer*>(this) : nullptr;
    }
};

// ============================================================================
// 具体层实现 (继承自 Layer)
// ============================================================================

class TileLayerImpl : public Layer {
public:
    TileLayer tileData;

    LayerType GetType() const override { return LayerType::TileLayer; }
    TileLayerImpl* AsTileLayer() { return this; }
};

class ObjectLayerImpl : public Layer {
public:
    ObjectLayer objectData;

    LayerType GetType() const override { return LayerType::ObjectLayer; }
    ObjectLayerImpl* AsObjectLayer() { return this; }
};

class ImageLayerImpl : public Layer {
public:
    ImageLayer imageData;

    LayerType GetType() const override { return LayerType::ImageLayer; }
    ImageLayerImpl* AsImageLayer() { return this; }
};

class GroupLayerImpl : public Layer {
public:
    GroupLayer groupData;

    LayerType GetType() const override { return LayerType::GroupLayer; }
    GroupLayerImpl* AsGroupLayer() { return this; }
};

// ============================================================================
// 地图根结构
// ============================================================================

struct TileMap {
    // 基本信息
    std::string version;           // TMX 格式版本
    std::string name;              // 地图名称 (来自文件名)
    std::string type = "map";      // 类型标识

    // 地图属性
    Orientation orientation = Orientation::Orthogonal;
    RenderOrder renderOrder = RenderOrder::RightDown;
    int width = 0;                 // 地图宽度 (瓦片)
    int height = 0;                // 地图高度 (瓦片)
    int tileWidth = 0;             // 瓦片像素宽度
    int tileHeight = 0;            // 瓦片像素高度
    int hexSideLength = 0;         // 六边形边长
    StaggerAxis staggerAxis = StaggerAxis::Y;
    StaggerIndex staggerIndex = StaggerIndex::Odd;

    // 无限地图
    bool infinite = false;

    // 背景色
    std::string backgroundColor;

    // 图块集
    std::vector<std::unique_ptr<Tileset>> tilesets;

    // 层
    std::vector<std::unique_ptr<Layer>> layers;

    // 自定义属性
    PropertyMap properties;

    // =========================================================================
    // 工具方法
    // =========================================================================

    // 获取总层数 (递归计算组层)
    size_t GetTotalLayerCount() const {
        size_t count = layers.size();
        for (const auto& layer : layers) {
            if (layer->GetType() == LayerType::GroupLayer) {
                // 组层的子层数量
                // 这里简化处理，实际可能需要递归
            }
        }
        return count;
    }

    // 查找层
    Layer* FindLayer(int layerId) const {
        for (const auto& layer : layers) {
            if (layer->id == layerId) {
                return layer.get();
            }
            if (layer->GetType() == LayerType::GroupLayer) {
                auto* group = static_cast<GroupLayerImpl*>(layer.get());
                // TODO: 递归搜索子层
            }
        }
        return nullptr;
    }

    Layer* FindLayerByName(const std::string& layerName) const {
        for (const auto& layer : layers) {
            if (layer->name == layerName) {
                return layer.get();
            }
            if (layer->GetType() == LayerType::GroupLayer) {
                // TODO: 递归搜索子层
            }
        }
        return nullptr;
    }

    // 根据 GID 查找图块集
    const Tileset* FindTilesetByGid(uint32_t gid) const {
        uint32_t pureGid = GIDHelper::GetPureGid(gid);
        for (const auto& tileset : tilesets) {
            if (tileset->ContainsGid(pureGid)) {
                return tileset.get();
            }
        }
        return nullptr;
    }

    // 获取所有瓦片层
    std::vector<TileLayer*> GetTileLayers() {
        std::vector<TileLayer*> result;
        CollectTileLayers(result, layers);
        return result;
    }

    // 获取所有对象层
    std::vector<ObjectLayer*> GetObjectLayers() {
        std::vector<ObjectLayer*> result;
        CollectObjectLayers(result, layers);
        return result;
    }

    // 获取所有图像层
    std::vector<ImageLayer*> GetImageLayers() {
        std::vector<ImageLayer*> result;
        CollectImageLayers(result, layers);
        return result;
    }

    // 检查是否为无限地图
    bool IsInfinite() const {
        return infinite;
    }

    // 获取地图像素尺寸
    int GetPixelWidth() const {
        return width * tileWidth;
    }

    int GetPixelHeight() const {
        return height * tileHeight;
    }

private:
    static void CollectTileLayers(std::vector<TileLayer*>& result, const std::vector<std::unique_ptr<Layer>>& layers) {
        for (const auto& layer : layers) {
            if (layer->GetType() == LayerType::TileLayer) {
                result.push_back(static_cast<TileLayerImpl*>(layer.get())->AsTileLayer());
            } else if (layer->GetType() == LayerType::GroupLayer) {
                // TODO: 递归
            }
        }
    }

    static void CollectObjectLayers(std::vector<ObjectLayer*>& result, const std::vector<std::unique_ptr<Layer>>& layers) {
        for (const auto& layer : layers) {
            if (layer->GetType() == LayerType::ObjectLayer) {
                result.push_back(static_cast<ObjectLayerImpl*>(layer.get())->AsObjectLayer());
            } else if (layer->GetType() == LayerType::GroupLayer) {
                // TODO: 递归
            }
        }
    }

    static void CollectImageLayers(std::vector<ImageLayer*>& result, const std::vector<std::unique_ptr<Layer>>& layers) {
        for (const auto& layer : layers) {
            if (layer->GetType() == LayerType::ImageLayer) {
                result.push_back(static_cast<ImageLayerImpl*>(layer.get())->AsImageLayer());
            } else if (layer->GetType() == LayerType::GroupLayer) {
                // TODO: 递归
            }
        }
    }
};

} // namespace PrismaEngine
