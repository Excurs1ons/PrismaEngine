#pragma once

#include "core/Map.h"
#include "../resource/Asset.h"
#include <filesystem>
#include <memory>

namespace PrismaEngine {

// ============================================================================
// 瓦片地图资源类
// ============================================================================

class TilemapAsset : public Asset {
public:
    TilemapAsset() = default;
    ~TilemapAsset() override = default;

    // AssetBase 接口实现
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override { return m_isLoaded; }
    AssetType GetType() const override { return AssetType::Tilemap; }

    // Serializable 接口实现
    void Serialize(OutputArchive& archive) const override;
    void Deserialize(InputArchive& archive) override;

    // 资产特定方法
    std::string GetAssetType() const override { return "Tilemap"; }
    std::string GetAssetVersion() const override { return "1.0.0"; }

    // 获取解析后的地图数据
    const TileMap* GetMap() const { return m_map.get(); }
    TileMap* GetMap() { return m_map.get(); }

    // 获取地图信息
    int GetWidth() const { return m_map ? m_map->width : 0; }
    int GetHeight() const { return m_map ? m_map->height : 0; }
    int GetTileWidth() const { return m_map ? m_map->tileWidth : 0; }
    int GetTileHeight() const { return m_map ? m_map->tileHeight : 0; }

    // 获取层数
    size_t GetLayerCount() const { return m_map ? m_map->layers.size() : 0; }

    // 获取图块集数量
    size_t GetTilesetCount() const { return m_map ? m_map->tilesets.size() : 0; }

    // 获取地图像素尺寸
    int GetPixelWidth() const { return m_map ? m_map->GetPixelWidth() : 0; }
    int GetPixelHeight() const { return m_map ? m_map->GetPixelHeight() : 0; }

    // 获取方向
    Orientation GetOrientation() const {
        return m_map ? m_map->orientation : Orientation::Orthogonal;
    }

    // 是否为无限地图
    bool IsInfinite() const { return m_map && m_map->infinite; }

    // 获取瓦片层
    std::vector<TileLayer*> GetTileLayers() const {
        return m_map ? m_map->GetTileLayers() : std::vector<TileLayer*>();
    }

    // 获取对象层
    std::vector<ObjectLayer*> GetObjectLayers() const {
        return m_map ? m_map->GetObjectLayers() : std::vector<ObjectLayer*>();
    }

    // 获取图像层
    std::vector<ImageLayer*> GetImageLayers() const {
        return m_map ? m_map->GetImageLayers() : std::vector<ImageLayer*>();
    }

    // 查找层
    Layer* FindLayer(int layerId) const {
        return m_map ? m_map->FindLayer(layerId) : nullptr;
    }

    Layer* FindLayerByName(const std::string& layerName) const {
        return m_map ? m_map->FindLayerByName(layerName) : nullptr;
    }

    // 根据 GID 查找图块集
    const Tileset* FindTilesetByGid(uint32_t gid) const {
        return m_map ? m_map->FindTilesetByGid(gid) : nullptr;
    }

    // 获取地图属性
    const PropertyMap& GetProperties() const {
        static const PropertyMap emptyProps;
        return m_map ? m_map->properties : emptyProps;
    }

    // 获取属性值
    template<typename T>
    T GetProperty(const std::string& name, const T& defaultValue) const;

private:
    std::unique_ptr<TileMap> m_map;
    bool m_isLoaded = false;
};

// ============================================================================
// 属性获取特化
// ============================================================================

template<>
inline std::string TilemapAsset::GetProperty<std::string>(
    const std::string& name,
    const std::string& defaultValue
) const {
    if (!m_map) return defaultValue;
    auto it = m_map->properties.find(name);
    if (it != m_map->properties.end()) {
        return it->second.AsString();
    }
    return defaultValue;
}

template<>
inline int TilemapAsset::GetProperty<int>(
    const std::string& name,
    const int& defaultValue
) const {
    if (!m_map) return defaultValue;
    auto it = m_map->properties.find(name);
    if (it != m_map->properties.end()) {
        return it->second.AsInt();
    }
    return defaultValue;
}

template<>
inline float TilemapAsset::GetProperty<float>(
    const std::string& name,
    const float& defaultValue
) const {
    if (!m_map) return defaultValue;
    auto it = m_map->properties.find(name);
    if (it != m_map->properties.end()) {
        return it->second.AsFloat();
    }
    return defaultValue;
}

template<>
inline bool TilemapAsset::GetProperty<bool>(
    const std::string& name,
    const bool& defaultValue
) const {
    if (!m_map) return defaultValue;
    auto it = m_map->properties.find(name);
    if (it != m_map->properties.end()) {
        return it->second.AsBool();
    }
    return defaultValue;
}

} // namespace PrismaEngine
