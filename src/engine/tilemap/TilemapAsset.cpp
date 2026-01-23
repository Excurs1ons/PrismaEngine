#include "TilemapAsset.h"
#include "format/TmxParser.h"
#include "format/TsxParser.h"
#include "../resource/AssetSerializer.h"
#include <filesystem>

namespace PrismaEngine {

// ============================================================================
// 加载
// ============================================================================

bool TilemapAsset::Load(const std::filesystem::path& path) {
    Unload();

    // 设置资源路径
    m_path = path;
    m_name = path.filename().string();

    // 解析 TMX 文件
    m_map = TmxParser::ParseFile(path);

    if (!m_map) {
        // 检查错误信息
        const std::string& error = TmxParser::GetLastError();
        if (!error.empty()) {
            // 记录错误
        }
        return false;
    }

    m_isLoaded = true;

    // 设置元数据
    SetMetadata(m_name, "Tilemap loaded from " + path.string());

    return true;
}

// ============================================================================
// 卸载
// ============================================================================

void TilemapAsset::Unload() {
    m_map.reset();
    m_isLoaded = false;
}

// ============================================================================
// 序列化
// ============================================================================

void TilemapAsset::Serialize(OutputArchive& archive) const {
    // 序列化基本信息
    archive("name", m_name);
    archive("path", m_path.string());

    if (m_map) {
        archive("version", m_map->version);
        archive("orientation", static_cast<int>(m_map->orientation));
        archive("renderOrder", static_cast<int>(m_map->renderOrder));
        archive("width", m_map->width);
        archive("height", m_map->height);
        archive("tileWidth", m_map->tileWidth);
        archive("tileHeight", m_map->tileHeight);
        archive("infinite", m_map->infinite);

        // 序列化图块集数量
        archive("tilesetCount", static_cast<int>(m_map->tilesets.size()));

        // TODO: 序列化完整地图数据
    }
}

// ============================================================================
// 反序列化
// ============================================================================

void TilemapAsset::Deserialize(InputArchive& archive) {
    // 反序列化基本信息
    archive("name", m_name);
    std::string pathStr;
    archive("path", pathStr);
    m_path = pathStr;

    // 创建空地图
    m_map = std::make_unique<TileMap>();

    archive("version", m_map->version);
    int orientationValue = 0;
    archive("orientation", orientationValue);
    m_map->orientation = static_cast<Orientation>(orientationValue);

    int renderOrderValue = 0;
    archive("renderOrder", renderOrderValue);
    m_map->renderOrder = static_cast<RenderOrder>(renderOrderValue);

    archive("width", m_map->width);
    archive("height", m_map->height);
    archive("tileWidth", m_map->tileWidth);
    archive("tileHeight", m_map->tileHeight);
    archive("infinite", m_map->infinite);

    // TODO: 反序列化完整地图数据

    m_isLoaded = true;
}

} // namespace PrismaEngine
