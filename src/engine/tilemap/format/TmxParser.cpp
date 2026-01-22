#include "TmxParser.h"
#include "TileDataDecoder.h"
#include <tinyxml2.h>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace PrismaEngine {

std::string TmxParser::s_lastError;

// ============================================================================
// 字符串转换工具
// ============================================================================

Orientation TmxParser::ParseOrientation(const std::string& str) {
    if (str == "isometric") return Orientation::Isometric;
    if (str == "staggered") return Orientation::Staggered;
    if (str == "hexagonal") return Orientation::Hexagonal;
    return Orientation::Orthogonal;
}

RenderOrder TmxParser::ParseRenderOrder(const std::string& str) {
    if (str == "right-up") return RenderOrder::RightUp;
    if (str == "left-down") return RenderOrder::LeftDown;
    if (str == "left-up") return RenderOrder::LeftUp;
    return RenderOrder::RightDown;
}

StaggerAxis TmxParser::ParseStaggerAxis(const std::string& str) {
    return (str == "x") ? StaggerAxis::X : StaggerAxis::Y;
}

StaggerIndex TmxParser::ParseStaggerIndex(const std::string& str) {
    return (str == "even") ? StaggerIndex::Even : StaggerIndex::Odd;
}

PropertyType TmxParser::ParsePropertyType(const std::string& str) {
    if (str == "int") return PropertyType::Int;
    if (str == "float") return PropertyType::Float;
    if (str == "bool") return PropertyType::Bool;
    if (str == "color") return PropertyType::Color;
    if (str == "file") return PropertyType::File;
    if (str == "object") return PropertyType::Object;
    if (str == "class") return PropertyType::Class;
    return PropertyType::String;
}

ObjectType TmxParser::ParseObjectType(const std::string& str) {
    if (str == "ellipse") return ObjectType::Ellipse;
    if (str == "point") return ObjectType::Point;
    if (str == "polygon") return ObjectType::Polygon;
    if (str == "polyline") return ObjectType::Polyline;
    if (str == "text") return ObjectType::Text;
    if (str == "tile") return ObjectType::Tile;
    return ObjectType::Rectangle;
}

DrawOrder TmxParser::ParseDrawOrder(const std::string& str) {
    return (str == "topdown") ? DrawOrder::Topdown : DrawOrder::Index;
}

LayerType TmxParser::ParseLayerType(const std::string& str) {
    if (str == "objectgroup") return LayerType::ObjectLayer;
    if (str == "imagelayer") return LayerType::ImageLayer;
    if (str == "group") return LayerType::GroupLayer;
    return LayerType::TileLayer;
}

// ============================================================================
// 颜色解析
// ============================================================================

std::tuple<int, int, int, int> TmxParser::ParseColor(const std::string& colorStr) {
    int r = 0, g = 0, b = 0, a = 255;

    if (!colorStr.empty() && colorStr[0] == '#') {
        std::string hex = colorStr.substr(1);

        if (hex.length() == 6) {
            // RRGGBB
            r = std::stoi(hex.substr(0, 2), nullptr, 16);
            g = std::stoi(hex.substr(2, 2), nullptr, 16);
            b = std::stoi(hex.substr(4, 2), nullptr, 16);
            a = 255;
        } else if (hex.length() == 8) {
            // AARRGGBB
            a = std::stoi(hex.substr(0, 2), nullptr, 16);
            r = std::stoi(hex.substr(2, 2), nullptr, 16);
            g = std::stoi(hex.substr(4, 2), nullptr, 16);
            b = std::stoi(hex.substr(6, 2), nullptr, 16);
        }
    }

    return std::make_tuple(r, g, b, a);
}

// ============================================================================
// 点解析
// ============================================================================

std::vector<std::pair<float, float>> TmxParser::ParsePoints(const std::string& pointsStr) {
    std::vector<std::pair<float, float>> points;
    std::istringstream stream(pointsStr);
    std::string pointStr;

    while (std::getline(stream, pointStr, ' ')) {
        if (!pointStr.empty()) {
            size_t commaPos = pointStr.find(',');
            if (commaPos != std::string::npos) {
                try {
                    float x = std::stof(pointStr.substr(0, commaPos));
                    float y = std::stof(pointStr.substr(commaPos + 1));
                    points.push_back({x, y});
                } catch (...) {
                    // 忽略无效点
                }
            }
        }
    }

    return points;
}

// ============================================================================
// 属性解析
// ============================================================================

PropertyMap TmxParser::ParseProperties(void* propertiesElement) {
    PropertyMap properties;
    tinyxml2::XMLElement* propsElem = static_cast<tinyxml2::XMLElement*>(propertiesElement);

    if (!propsElem) return properties;

    for (tinyxml2::XMLElement* propElem = propsElem->FirstChildElement("property");
         propElem != nullptr;
         propElem = propElem->NextSiblingElement("property")) {

        Property prop;
        prop.name = propElem->Attribute("name") ? propElem->Attribute("name") : "";

        const char* typeAttr = propElem->Attribute("type");
        prop.type = typeAttr ? ParsePropertyType(typeAttr) : PropertyType::String;

        const char* valueAttr = propElem->Attribute("value");

        if (valueAttr) {
            prop.value = valueAttr;
        } else {
            // 对于某些类型，值可能在元素内容中
            if (const char* text = propElem->GetText()) {
                prop.value = text;
            }
        }

        properties[prop.name] = prop;
    }

    return properties;
}

// ============================================================================
// 动画帧解析
// ============================================================================

std::vector<Frame> TmxParser::ParseAnimation(void* animationElement) {
    std::vector<Frame> frames;
    tinyxml2::XMLElement* animElem = static_cast<tinyxml2::XMLElement*>(animationElement);

    if (!animElem) return frames;

    for (tinyxml2::XMLElement* frameElem = animElem->FirstChildElement("frame");
         frameElem != nullptr;
         frameElem = frameElem->NextSiblingElement("frame")) {

        Frame frame;
        frame.tileId = frameElem->IntAttribute("tileid", 0);
        frame.duration = frameElem->IntAttribute("duration", 0);
        frames.push_back(frame);
    }

    return frames;
}

// ============================================================================
// 碰撞形状解析
// ============================================================================

std::vector<CollisionShape> TmxParser::ParseCollisionShapes(void* tileElement) {
    std::vector<CollisionShape> shapes;
    tinyxml2::XMLElement* elem = static_cast<tinyxml2::XMLElement*>(tileElement);

    if (!elem) return shapes;

    for (tinyxml2::XMLElement* objElem = elem->FirstChildElement("object");
         objElem != nullptr;
         objElem = objElem->NextSiblingElement("object")) {

        CollisionShape shape;

        const char* typeAttr = objElem->Attribute("type");
        if (!typeAttr || strcmp(typeAttr, "") == 0) {
            // 没有类型属性，检查子元素来确定类型
            if (objElem->FirstChildElement("ellipse")) {
                shape.type = CollisionShapeType::Ellipse;
            } else if (objElem->FirstChildElement("polygon")) {
                shape.type = CollisionShapeType::Polygon;
            } else if (objElem->FirstChildElement("polyline")) {
                shape.type = CollisionShapeType::Polyline;
            } else {
                shape.type = CollisionShapeType::Rectangle;
            }
        } else {
            if (strcmp(typeAttr, "ellipse") == 0) {
                shape.type = CollisionShapeType::Ellipse;
            } else if (strcmp(typeAttr, "polygon") == 0) {
                shape.type = CollisionShapeType::Polygon;
            } else if (strcmp(typeAttr, "polyline") == 0) {
                shape.type = CollisionShapeType::Polyline;
            } else {
                shape.type = CollisionShapeType::Rectangle;
            }
        }

        // 解析点
        tinyxml2::XMLElement* polyElem = objElem->FirstChildElement("polygon");
        tinyxml2::XMLElement* lineElem = objElem->FirstChildElement("polyline");

        if (polyElem) {
            const char* pointsStr = polyElem->Attribute("points");
            if (pointsStr) {
                shape.points = ParsePoints(pointsStr);
            }
        } else if (lineElem) {
            const char* pointsStr = lineElem->Attribute("points");
            if (pointsStr) {
                shape.points = ParsePoints(pointsStr);
            }
        } else {
            // 矩形
            float x = objElem->FloatAttribute("x", 0.0f);
            float y = objElem->FloatAttribute("y", 0.0f);
            float w = objElem->FloatAttribute("width", 0.0f);
            float h = objElem->FloatAttribute("height", 0.0f);
            shape.points = {{0, 0}, {static_cast<int>(w), 0}, {static_cast<int>(w), static_cast<int>(h)}, {0, static_cast<int>(h)}};
        }

        shapes.push_back(shape);
    }

    return shapes;
}

// ============================================================================
// 文本对象解析
// ============================================================================

std::unique_ptr<TextObject> TmxParser::ParseText(void* textElement) {
    tinyxml2::XMLElement* textElem = static_cast<tinyxml2::XMLElement*>(textElement);

    if (!textElem) return nullptr;

    auto textObj = std::make_unique<TextObject>();

    if (const char* text = textElem->GetText()) {
        textObj->text = text;
    }

    textObj->fontFamily = textElem->Attribute("fontfamily") ? textElem->Attribute("fontfamily") : "sans-serif";
    textObj->pixelSize = textElem->IntAttribute("pixelsize", 16);
    textObj->wrap = textElem->BoolAttribute("wrap", false);
    textObj->color = textElem->Attribute("color") ? textElem->Attribute("color") : "#000000";
    textObj->bold = textElem->BoolAttribute("bold", false);
    textObj->italic = textElem->BoolAttribute("italic", false);
    textObj->underline = textElem->BoolAttribute("underline", false);
    textObj->strikeout = textElem->BoolAttribute("strikeout", false);
    textObj->kerning = textElem->IntAttribute("kerning", 0);

    // 对齐属性
    textObj->hAlign = textElem->Attribute("halign") && strcmp(textElem->Attribute("halign"), "center") == 0;
    textObj->vAlign = textElem->Attribute("valign") && strcmp(textElem->Attribute("valign"), "center") == 0;

    return textObj;
}

// ============================================================================
// 对象解析
// ============================================================================

std::unique_ptr<MapObject> TmxParser::ParseObject(void* objectElement) {
    tinyxml2::XMLElement* objElem = static_cast<tinyxml2::XMLElement*>(objectElement);

    if (!objElem) return nullptr;

    auto mapObj = std::make_unique<MapObject>();

    mapObj->id = objElem->IntAttribute("id", 0);
    mapObj->name = objElem->Attribute("name") ? objElem->Attribute("name") : "";
    mapObj->type = objElem->Attribute("type") ? objElem->Attribute("type") : "";
    mapObj->x = objElem->FloatAttribute("x", 0.0f);
    mapObj->y = objElem->FloatAttribute("y", 0.0f);
    mapObj->width = objElem->FloatAttribute("width", 0.0f);
    mapObj->height = objElem->FloatAttribute("height", 0.0f);
    mapObj->rotation = objElem->FloatAttribute("rotation", 0.0f);
    mapObj->gid = static_cast<uint32_t>(objElem->Int64Attribute("gid", 0));
    mapObj->visible = objElem->BoolAttribute("visible", true);

    // 确定对象类型
    if (objElem->FirstChildElement("ellipse")) {
        mapObj->objectType = ObjectType::Ellipse;
    } else if (objElem->FirstChildElement("point")) {
        mapObj->objectType = ObjectType::Point;
    } else if (objElem->FirstChildElement("polygon")) {
        mapObj->objectType = ObjectType::Polygon;
        tinyxml2::XMLElement* polyElem = objElem->FirstChildElement("polygon");
        const char* pointsStr = polyElem->Attribute("points");
        if (pointsStr) {
            mapObj->points = ParsePoints(pointsStr);
        }
    } else if (objElem->FirstChildElement("polyline")) {
        mapObj->objectType = ObjectType::Polyline;
        tinyxml2::XMLElement* lineElem = objElem->FirstChildElement("polyline");
        const char* pointsStr = lineElem->Attribute("points");
        if (pointsStr) {
            mapObj->points = ParsePoints(pointsStr);
        }
    } else if (objElem->FirstChildElement("text")) {
        mapObj->objectType = ObjectType::Text;
        mapObj->text = ParseText(objElem->FirstChildElement("text")).release();
    } else if (mapObj->gid != 0) {
        mapObj->objectType = ObjectType::Tile;
    } else {
        mapObj->objectType = ObjectType::Rectangle;
    }

    // 解析属性
    tinyxml2::XMLElement* propsElem = objElem->FirstChildElement("properties");
    if (propsElem) {
        mapObj->properties = ParseProperties(propsElem);
    }

    return mapObj;
}

// ============================================================================
// 瓦片层数据解析
// ============================================================================

bool TmxParser::ParseTileData(
    void* dataElement,
    TileLayerData& outData,
    int width,
    int height
) {
    tinyxml2::XMLElement* dataElem = static_cast<tinyxml2::XMLElement*>(dataElement);

    if (!dataElem) return false;

    // 检查编码方式
    const char* encoding = dataElem->Attribute("encoding");
    const char* compression = dataElem->Attribute("compression");

    TileDataEncoding dataEncoding = TileDataEncoding::CSV;

    if (!encoding) {
        // 默认 CSV (无 encoding 属性时，可能是 XML 格式的瓦片)
        // 检查是否有 tile 子元素
        if (dataElem->FirstChildElement("tile")) {
            // XML 格式 - 每个瓦片是一个元素
            std::vector<uint32_t> gids;
            for (tinyxml2::XMLElement* tileElem = dataElem->FirstChildElement("tile");
                 tileElem != nullptr;
                 tileElem = tileElem->NextSiblingElement("tile")) {
                uint32_t gid = static_cast<uint32_t>(tileElem->IntAttribute("gid", 0));
                gids.push_back(gid);
            }
            outData.data = gids;
            outData.width = width;
            outData.height = height;
            return true;
        }
    } else if (strcmp(encoding, "csv") == 0) {
        dataEncoding = TileDataEncoding::CSV;
    } else if (strcmp(encoding, "base64") == 0) {
        if (!compression) {
            dataEncoding = TileDataEncoding::Base64;
        } else if (strcmp(compression, "zlib") == 0) {
            dataEncoding = TileDataEncoding::Base64_Zlib;
        } else if (strcmp(compression, "zstd") == 0) {
            dataEncoding = TileDataEncoding::Base64_Zstd;
        } else if (strcmp(compression, "gzip") == 0) {
            dataEncoding = TileDataEncoding::Base64_Gzip;
        } else {
            s_lastError = "Unknown compression format: " + std::string(compression);
            return false;
        }
    } else {
        s_lastError = "Unknown encoding format: " + std::string(encoding);
        return false;
    }

    // 获取数据内容
    const char* text = dataElem->GetText();
    if (!text) {
        outData.width = width;
        outData.height = height;
        outData.data.clear();
        return true;
    }

    // 解码数据
    outData.data = TileDataDecoder::Decode(text, dataEncoding, width * height);
    outData.width = width;
    outData.height = height;

    return true;
}

// ============================================================================
// 图块集解析
// ============================================================================

std::unique_ptr<Tileset> TmxParser::ParseTileset(
    void* tilesetElement,
    const std::filesystem::path& basePath
) {
    tinyxml2::XMLElement* tsElem = static_cast<tinyxml2::XMLElement*>(tilesetElement);

    if (!tsElem) return nullptr;

    auto tileset = std::make_unique<Tileset>();

    // 基本信息
    tileset->firstGid = tsElem->IntAttribute("firstgid", 0);
    const char* source = tsElem->Attribute("source");

    if (source) {
        // 外部 TSX 文件
        tileset->source = source;

        // TODO: 加载外部 TSX 文件
        // 目前跳过，需要在完整实现中处理
        return tileset;
    }

    tileset->name = tsElem->Attribute("name") ? tsElem->Attribute("name") : "";
    tileset->tileWidth = tsElem->IntAttribute("tilewidth", 0);
    tileset->tileHeight = tsElem->IntAttribute("tileheight", 0);
    tileset->spacing = tsElem->IntAttribute("spacing", 0);
    tileset->margin = tsElem->IntAttribute("margin", 0);
    tileset->tileCount = tsElem->IntAttribute("tilecount", 0);
    tileset->columns = tsElem->IntAttribute("columns", 0);
    tileset->objectAlignment = tsElem->IntAttribute("objectalignment", 0);

    // 图像信息
    tinyxml2::XMLElement* imgElem = tsElem->FirstChildElement("image");
    if (imgElem) {
        tileset->imagePath = imgElem->Attribute("source") ? imgElem->Attribute("source") : "";
        tileset->imageWidth = imgElem->IntAttribute("width", 0);
        tileset->imageHeight = imgElem->IntAttribute("height", 0);

        const char* transAttr = imgElem->Attribute("trans");
        if (transAttr) {
            auto [r, g, b, a] = ParseColor(std::string("#") + transAttr);
            tileset->transparentColor.r = r;
            tileset->transparentColor.g = g;
            tileset->transparentColor.b = b;
        }
    }

    // Image Collection 模式
    for (tinyxml2::XMLElement* tileImgElem = tsElem->FirstChildElement("tile");
         tileImgElem != nullptr;
         tileImgElem = tileImgElem->NextSiblingElement("tile")) {

        int id = tileImgElem->IntAttribute("id", -1);
        if (id < 0) continue;

        tinyxml2::XMLElement* imgInTile = tileImgElem->FirstChildElement("image");
        if (imgInTile) {
            Tileset::ImageTile imgTile;
            imgTile.id = id;
            imgTile.imagePath = imgInTile->Attribute("source") ? imgInTile->Attribute("source") : "";
            imgTile.imageWidth = imgInTile->IntAttribute("width", 0);
            imgTile.imageHeight = imgInTile->IntAttribute("height", 0);
            tileset->images.push_back(imgTile);
        }
    }

    // 瓦片偏移
    tinyxml2::XMLElement* offsetElem = tsElem->FirstChildElement("tileoffset");
    if (offsetElem) {
        tileset->tileOffset.x = offsetElem->IntAttribute("x", 0);
        tileset->tileOffset.y = offsetElem->IntAttribute("y", 0);
    }

    // 网格 (六边形)
    tinyxml2::XMLElement* gridElem = tsElem->FirstChildElement("grid");
    if (gridElem) {
        const char* orientation = gridElem->Attribute("orientation");
        if (orientation) {
            tileset->grid.orientation = ParseOrientation(orientation);
        }
        tileset->grid.width = gridElem->IntAttribute("width", 0);
        tileset->grid.height = gridElem->IntAttribute("height", 0);
    }

    // 属性
    tinyxml2::XMLElement* propsElem = tsElem->FirstChildElement("properties");
    if (propsElem) {
        tileset->properties = ParseProperties(propsElem);
    }

    // 特殊瓦片
    for (tinyxml2::XMLElement* tileElem = tsElem->FirstChildElement("tile");
         tileElem != nullptr;
         tileElem = tileElem->NextSiblingElement("tile")) {

        int id = tileElem->IntAttribute("id", -1);
        if (id < 0) continue;

        Tile tile;
        tile.id = id;

        tile.type = tileElem->Attribute("type") ? tileElem->Attribute("type") : "";
        tile.probability = tileElem->FloatAttribute("probability", 1.0f);

        // 地形信息
        tile.terrainTopLeft = tileElem->IntAttribute("terrain", -1);

        // 动画
        tinyxml2::XMLElement* animElem = tileElem->FirstChildElement("animation");
        if (animElem) {
            tile.animation = ParseAnimation(animElem);
        }

        // 碰撞形状
        tinyxml2::XMLElement* objGroupElem = tileElem->FirstChildElement("objectgroup");
        if (objGroupElem) {
            tile.collisionShapes = ParseCollisionShapes(tileElem);
        }

        // 属性
        tinyxml2::XMLElement* tilePropsElem = tileElem->FirstChildElement("properties");
        if (tilePropsElem) {
            tile.properties = ParseProperties(tilePropsElem);
        }

        // 图像 (Image Collection)
        tinyxml2::XMLElement* imgElem = tileElem->FirstChildElement("image");
        if (imgElem) {
            tile.imagePath = imgElem->Attribute("source") ? imgElem->Attribute("source") : "";
        }

        tileset->tiles[id] = tile;
    }

    // Wang sets (用于自动地形)
    for (tinyxml2::XMLElement* wangElem = tsElem->FirstChildElement("wangsets");
         wangElem != nullptr;
         wangElem = wangElem->NextSiblingElement("wangsets")) {

        for (tinyxml2::XMLElement* wangSetElem = wangElem->FirstChildElement("wangset");
             wangSetElem != nullptr;
             wangSetElem = wangSetElem->NextSiblingElement("wangset")) {

            WangSet wangSet;
            wangSet.name = wangSetElem->Attribute("name") ? wangSetElem->Attribute("name") : "";
            wangSet.tile = wangSetElem->IntAttribute("tile", -1);

            tileset->wangSets.push_back(wangSet);
        }
    }

    return tileset;
}

// ============================================================================
// 层解析
// ============================================================================

std::unique_ptr<Layer> TmxParser::ParseLayer(
    void* layerElement,
    const std::filesystem::path& basePath,
    TileMap& map
) {
    tinyxml2::XMLElement* layerElem = static_cast<tinyxml2::XMLElement*>(layerElement);

    if (!layerElem) return nullptr;

    // 确定层类型
    const char* nameAttr = layerElem->Name();
    LayerType layerType = ParseLayerType(nameAttr ? nameAttr : "");

    int id = layerElem->IntAttribute("id", 0);
    std::string name = layerElem->Attribute("name") ? layerElem->Attribute("name") : "";
    bool visible = layerElem->BoolAttribute("visible", true);
    float opacity = layerElem->FloatAttribute("opacity", 1.0f);
    int offsetX = layerElem->IntAttribute("offsetx", 0);
    int offsetY = layerElem->IntAttribute("offsety", 0);
    float parallaxX = layerElem->FloatAttribute("parallaxx", 1.0f);
    float parallaxY = layerElem->FloatAttribute("parallaxy", 1.0f);
    std::string tint = layerElem->Attribute("tintcolor") ? layerElem->Attribute("tintcolor") : "";

    switch (layerType) {
        case LayerType::TileLayer: {
            auto tileLayerImpl = std::make_unique<TileLayerImpl>();
            auto* tileLayer = tileLayerImpl->AsTileLayer();

            tileLayer->id = id;
            tileLayer->name = name;
            tileLayer->visible = visible;
            tileLayer->opacity = opacity;
            tileLayer->offsetX = offsetX;
            tileLayer->offsetY = offsetY;
            tileLayer->parallaxX = parallaxX;
            tileLayer->parallaxY = parallaxY;
            tileLayer->tint = tint;

            // 解析瓦片数据
            int width = layerElem->IntAttribute("width", 0);
            int height = layerElem->IntAttribute("height", 0);

            tinyxml2::XMLElement* dataElem = layerElem->FirstChildElement("data");
            if (dataElem) {
                ParseTileData(dataElem, tileLayer->tileData, width, height);
            }

            // 解析块数据 (无限地图)
            for (tinyxml2::XMLElement* chunkElem = dataElem ? dataElem->FirstChildElement("chunk") : nullptr;
                 chunkElem != nullptr;
                 chunkElem = chunkElem->NextSiblingElement("chunk")) {

                TileLayerData::Chunk chunk;
                chunk.x = chunkElem->IntAttribute("x", 0);
                chunk.y = chunkElem->IntAttribute("y", 0);
                chunk.width = chunkElem->IntAttribute("width", 0);
                chunk.height = chunkElem->IntAttribute("height", 0);

                // 解析块数据
                const char* encoding = chunkElem->Attribute("encoding");
                const char* compression = chunkElem->Attribute("compression");

                TileDataEncoding dataEncoding = TileDataEncoding::CSV;
                if (encoding && strcmp(encoding, "base64") == 0) {
                    if (!compression || strcmp(compression, "zlib") == 0) {
                        dataEncoding = TileDataEncoding::Base64_Zlib;
                    } else if (strcmp(compression, "zstd") == 0) {
                        dataEncoding = TileDataEncoding::Base64_Zstd;
                    } else if (strcmp(compression, "gzip") == 0) {
                        dataEncoding = TileDataEncoding::Base64_Gzip;
                    } else {
                        dataEncoding = TileDataEncoding::Base64;
                    }
                }

                const char* text = chunkElem->GetText();
                if (text) {
                    chunk.data = TileDataDecoder::Decode(text, dataEncoding, chunk.width * chunk.height);
                }

                tileLayer->tileData.chunks.push_back(chunk);
            }

            // 属性
            tinyxml2::XMLElement* propsElem = layerElem->FirstChildElement("properties");
            if (propsElem) {
                tileLayer->properties = ParseProperties(propsElem);
            }

            return std::move(tileLayerImpl);
        }

        case LayerType::ObjectLayer: {
            auto objLayerImpl = std::make_unique<ObjectLayerImpl>();
            auto* objLayer = objLayerImpl->AsObjectLayer();

            objLayer->id = id;
            objLayer->name = name;
            objLayer->visible = visible;
            objLayer->opacity = opacity;
            objLayer->offsetX = offsetX;
            objLayer->offsetY = offsetY;
            objLayer->parallaxX = parallaxX;
            objLayer->parallaxY = parallaxY;
            objLayer->tint = tint;

            // 绘制顺序
            const char* drawOrder = layerElem->Attribute("draworder");
            if (drawOrder) {
                objLayer->drawOrder = ParseDrawOrder(drawOrder);
            }

            // 解析对象
            for (tinyxml2::XMLElement* objElem = layerElem->FirstChildElement("object");
                 objElem != nullptr;
                 objElem = objElem->NextSiblingElement("object")) {

                auto mapObj = ParseObject(objElem);
                if (mapObj) {
                    objLayer->objects.push_back(std::move(mapObj));
                }
            }

            // 属性
            tinyxml2::XMLElement* propsElem = layerElem->FirstChildElement("properties");
            if (propsElem) {
                objLayer->properties = ParseProperties(propsElem);
            }

            return std::move(objLayerImpl);
        }

        case LayerType::ImageLayer: {
            auto imgLayerImpl = std::make_unique<ImageLayerImpl>();
            auto* imgLayer = imgLayerImpl->AsImageLayer();

            imgLayer->id = id;
            imgLayer->name = name;
            imgLayer->visible = visible;
            imgLayer->opacity = opacity;
            imgLayer->offsetX = offsetX;
            imgLayer->offsetY = offsetY;
            imgLayer->parallaxX = parallaxX;
            imgLayer->parallaxY = parallaxY;
            imgLayer->tint = tint;

            // 图像信息
            tinyxml2::XMLElement* imgElem = layerElem->FirstChildElement("image");
            if (imgElem) {
                imgLayer->imagePath = imgElem->Attribute("source") ? imgElem->Attribute("source") : "";
                imgLayer->imageWidth = imgElem->IntAttribute("width", 0);
                imgLayer->imageHeight = imgElem->IntAttribute("height", 0);
            }

            // 属性
            tinyxml2::XMLElement* propsElem = layerElem->FirstChildElement("properties");
            if (propsElem) {
                imgLayer->properties = ParseProperties(propsElem);
            }

            return std::move(imgLayerImpl);
        }

        case LayerType::GroupLayer: {
            auto groupLayerImpl = std::make_unique<GroupLayerImpl>();
            auto* groupLayer = groupLayerImpl->AsGroupLayer();

            groupLayer->id = id;
            groupLayer->name = name;
            groupLayer->visible = visible;
            groupLayer->opacity = opacity;
            groupLayer->offsetX = offsetX;
            groupLayer->offsetY = offsetY;
            groupLayer->parallaxX = parallaxX;
            groupLayer->parallaxY = parallaxY;
            groupLayer->tint = tint;

            // 解析子层
            for (tinyxml2::XMLElement* subLayerElem = layerElem->FirstChildElement();
                 subLayerElem != nullptr;
                 subLayerElem = subLayerElem->NextSiblingElement()) {

                std::string elemName = subLayerElem->Name();
                if (elemName == "properties") continue;

                auto subLayer = ParseLayer(subLayerElem, basePath, map);
                if (subLayer) {
                    groupLayer->groupData.layers.push_back(std::move(subLayer));
                }
            }

            // 属性
            tinyxml2::XMLElement* propsElem = layerElem->FirstChildElement("properties");
            if (propsElem) {
                groupLayer->properties = ParseProperties(propsElem);
            }

            return std::move(groupLayerImpl);
        }

        default:
            return nullptr;
    }
}

// ============================================================================
// 地图属性解析
// ============================================================================

bool TmxParser::ParseMapAttributes(void* mapElement, TileMap& outMap) {
    tinyxml2::XMLElement* mapElem = static_cast<tinyxml2::XMLElement*>(mapElement);

    if (!mapElem) return false;

    // 基本信息
    outMap.version = mapElem->Attribute("version") ? mapElem->Attribute("version") : "1.0";

    const char* orientation = mapElem->Attribute("orientation");
    if (orientation) {
        outMap.orientation = ParseOrientation(orientation);
    }

    const char* renderOrder = mapElem->Attribute("renderorder");
    if (renderOrder) {
        outMap.renderOrder = ParseRenderOrder(renderOrder);
    }

    outMap.width = mapElem->IntAttribute("width", 0);
    outMap.height = mapElem->IntAttribute("height", 0);
    outMap.tileWidth = mapElem->IntAttribute("tilewidth", 0);
    outMap.tileHeight = mapElem->IntAttribute("tileheight", 0);
    outMap.hexSideLength = mapElem->IntAttribute("hexsidelength", 0);
    outMap.infinite = mapElem->BoolAttribute("infinite", false);

    const char* staggerAxis = mapElem->Attribute("staggeraxis");
    if (staggerAxis) {
        outMap.staggerAxis = ParseStaggerAxis(staggerAxis);
    }

    const char* staggerIndex = mapElem->Attribute("staggerindex");
    if (staggerIndex) {
        outMap.staggerIndex = ParseStaggerIndex(staggerIndex);
    }

    outMap.backgroundColor = mapElem->Attribute("backgroundcolor") ?
        mapElem->Attribute("backgroundcolor") : "";

    // 自定义属性
    tinyxml2::XMLElement* propsElem = mapElem->FirstChildElement("properties");
    if (propsElem) {
        outMap.properties = ParseProperties(propsElem);
    }

    return true;
}

// ============================================================================
// 主解析方法
// ============================================================================

std::unique_ptr<TileMap> TmxParser::ParseFile(const std::filesystem::path& filePath) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(filePath.string().c_str());

    if (result != tinyxml2::XML_SUCCESS) {
        s_lastError = "Failed to load TMX file: " + filePath.string();
        return nullptr;
    }

    std::filesystem::path basePath = filePath.parent_path();

    tinyxml2::XMLElement* mapElem = doc.FirstChildElement("map");
    if (!mapElem) {
        s_lastError = "No map element found in TMX file";
        return nullptr;
    }

    auto map = std::make_unique<TileMap>();
    map->name = filePath.stem().string();

    if (!ParseMapAttributes(mapElem, *map)) {
        return nullptr;
    }

    // 解析图块集
    for (tinyxml2::XMLElement* tsElem = mapElem->FirstChildElement("tileset");
         tsElem != nullptr;
         tsElem = tsElem->NextSiblingElement("tileset")) {

        auto tileset = ParseTileset(tsElem, basePath);
        if (tileset) {
            map->tilesets.push_back(std::move(tileset));
        }
    }

    // 解析层
    for (tinyxml2::XMLElement* layerElem = mapElem->FirstChildElement();
         layerElem != nullptr;
         layerElem = layerElem->NextSiblingElement()) {

        std::string elemName = layerElem->Name();
        if (elemName == "properties" || elemName == "tileset") continue;

        auto layer = ParseLayer(layerElem, basePath, *map);
        if (layer) {
            map->layers.push_back(std::move(layer));
        }
    }

    return map;
}

std::unique_ptr<TileMap> TmxParser::ParseString(const std::string& tmxContent) {
    return ParseString(tmxContent, std::filesystem::path());
}

std::unique_ptr<TileMap> TmxParser::ParseString(
    const std::string& tmxContent,
    const std::filesystem::path& basePath
) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.Parse(tmxContent.c_str());

    if (result != tinyxml2::XML_SUCCESS) {
        s_lastError = "Failed to parse TMX content";
        return nullptr;
    }

    tinyxml2::XMLElement* mapElem = doc.FirstChildElement("map");
    if (!mapElem) {
        s_lastError = "No map element found in TMX content";
        return nullptr;
    }

    auto map = std::make_unique<TileMap>();

    if (!ParseMapAttributes(mapElem, *map)) {
        return nullptr;
    }

    // 解析图块集
    for (tinyxml2::XMLElement* tsElem = mapElem->FirstChildElement("tileset");
         tsElem != nullptr;
         tsElem = tsElem->NextSiblingElement("tileset")) {

        auto tileset = ParseTileset(tsElem, basePath);
        if (tileset) {
            map->tilesets.push_back(std::move(tileset));
        }
    }

    // 解析层
    for (tinyxml2::XMLElement* layerElem = mapElem->FirstChildElement();
         layerElem != nullptr;
         layerElem = layerElem->NextSiblingElement()) {

        std::string elemName = layerElem->Name();
        if (elemName == "properties" || elemName == "tileset") continue;

        auto layer = ParseLayer(layerElem, basePath, *map);
        if (layer) {
            map->layers.push_back(std::move(layer));
        }
    }

    return map;
}

} // namespace PrismaEngine
