#pragma once

#include "../core/Map.h"
#include <string>
#include <memory>
#include <filesystem>

namespace PrismaEngine {

// ============================================================================
// TMX 解析器
// ============================================================================

class TmxParser {
public:
    // 解析 TMX 文件
    static std::unique_ptr<TileMap> ParseFile(const std::filesystem::path& filePath);

    // 解析 TMX 内容字符串
    static std::unique_ptr<TileMap> ParseString(const std::string& tmxContent);

    // 解析 TMX 内容 (带基础路径用于解析相对路径)
    static std::unique_ptr<TileMap> ParseString(
        const std::string& tmxContent,
        const std::filesystem::path& basePath
    );

    // 获取最后错误信息
    static const std::string& GetLastError() { return s_lastError; }

private:
    static std::string s_lastError;

    // 解析地图属性
    static bool ParseMapAttributes(void* mapElement, TileMap& outMap);

    // 解析图块集
    static std::unique_ptr<Tileset> ParseTileset(
        void* tilesetElement,
        const std::filesystem::path& basePath
    );

    // 解析层
    static std::unique_ptr<Layer> ParseLayer(
        void* layerElement,
        const std::filesystem::path& basePath,
        TileMap& map
    );

    // 解析瓦片层数据
    static bool ParseTileData(
        void* dataElement,
        TileLayerData& outData,
        int width,
        int height
    );

    // 解析对象
    static std::unique_ptr<MapObject> ParseObject(void* objectElement);

    // 解析属性
    static PropertyMap ParseProperties(void* propertiesElement);

    // 解析动画帧
    static std::vector<Frame> ParseAnimation(void* animationElement);

    // 解析碰撞形状
    static std::vector<CollisionShape> ParseCollisionShapes(void* tileElement);

    // 解析文本对象
    static std::unique_ptr<TextObject> ParseText(void* textElement);

    // 解析多边形/折线点
    static std::vector<std::pair<float, float>> ParsePoints(const std::string& pointsStr);

    // 解析颜色
    static std::tuple<int, int, int, int> ParseColor(const std::string& colorStr);

    // 字符串转换工具
    static Orientation ParseOrientation(const std::string& str);
    static RenderOrder ParseRenderOrder(const std::string& str);
    static StaggerAxis ParseStaggerAxis(const std::string& str);
    static StaggerIndex ParseStaggerIndex(const std::string& str);
    static PropertyType ParsePropertyType(const std::string& str);
    static ObjectType ParseObjectType(const std::string& str);
    static DrawOrder ParseDrawOrder(const std::string& str);
    static LayerType ParseLayerType(const std::string& str);
};

} // namespace PrismaEngine
