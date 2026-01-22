#include "TsxParser.h"
#include <tinyxml2.h>
#include <sstream>

namespace PrismaEngine {

std::string TsxParser::s_lastError;

// ============================================================================
// 属性解析
// ============================================================================

PropertyMap TsxParser::ParseProperties(void* propertiesElement) {
    PropertyMap properties;
    tinyxml2::XMLElement* propsElem = static_cast<tinyxml2::XMLElement*>(propertiesElement);

    if (!propsElem) return properties;

    for (tinyxml2::XMLElement* propElem = propsElem->FirstChildElement("property");
         propElem != nullptr;
         propElem = propElem->NextSiblingElement("property")) {

        Property prop;
        prop.name = propElem->Attribute("name") ? propElem->Attribute("name") : "";

        const char* typeAttr = propElem->Attribute("type");
        if (typeAttr) {
            std::string typeStr = typeAttr;
            if (typeStr == "int") prop.type = PropertyType::Int;
            else if (typeStr == "float") prop.type = PropertyType::Float;
            else if (typeStr == "bool") prop.type = PropertyType::Bool;
            else if (typeStr == "color") prop.type = PropertyType::Color;
            else if (typeStr == "file") prop.type = PropertyType::File;
            else if (typeStr == "object") prop.type = PropertyType::Object;
            else if (typeStr == "class") prop.type = PropertyType::Class;
            else prop.type = PropertyType::String;
        }

        const char* valueAttr = propElem->Attribute("value");
        if (valueAttr) {
            prop.value = valueAttr;
        } else {
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

std::vector<Frame> TsxParser::ParseAnimation(void* animationElement) {
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

std::vector<CollisionShape> TsxParser::ParseCollisionShapes(void* tileElement) {
    std::vector<CollisionShape> shapes;
    tinyxml2::XMLElement* elem = static_cast<tinyxml2::XMLElement*>(tileElement);

    if (!elem) return shapes;

    for (tinyxml2::XMLElement* objElem = elem->FirstChildElement("object");
         objElem != nullptr;
         objElem = objElem->NextSiblingElement("object")) {

        CollisionShape shape;

        const char* typeAttr = objElem->Attribute("type");
        if (!typeAttr || strcmp(typeAttr, "") == 0) {
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
                std::istringstream stream(pointsStr);
                std::string pointStr;
                while (std::getline(stream, pointStr, ' ')) {
                    if (!pointStr.empty()) {
                        size_t commaPos = pointStr.find(',');
                        if (commaPos != std::string::npos) {
                            try {
                                float x = std::stof(pointStr.substr(0, commaPos));
                                float y = std::stof(pointStr.substr(commaPos + 1));
                                shape.points.push_back({static_cast<int>(x), static_cast<int>(y)});
                            } catch (...) {}
                        }
                    }
                }
            }
        } else if (lineElem) {
            const char* pointsStr = lineElem->Attribute("points");
            if (pointsStr) {
                std::istringstream stream(pointsStr);
                std::string pointStr;
                while (std::getline(stream, pointStr, ' ')) {
                    if (!pointStr.empty()) {
                        size_t commaPos = pointStr.find(',');
                        if (commaPos != std::string::npos) {
                            try {
                                float x = std::stof(pointStr.substr(0, commaPos));
                                float y = std::stof(pointStr.substr(commaPos + 1));
                                shape.points.push_back({static_cast<int>(x), static_cast<int>(y)});
                            } catch (...) {}
                        }
                    }
                }
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
// 主解析方法
// ============================================================================

std::unique_ptr<Tileset> TsxParser::ParseFile(const std::filesystem::path& filePath) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(filePath.string().c_str());

    if (result != tinyxml2::XML_SUCCESS) {
        s_lastError = "Failed to load TSX file: " + filePath.string();
        return nullptr;
    }

    tinyxml2::XMLElement* tsElem = doc.FirstChildElement("tileset");
    if (!tsElem) {
        s_lastError = "No tileset element found in TSX file";
        return nullptr;
    }

    auto tileset = std::make_unique<Tileset>();

    // 基本信息
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
            std::string trans = "#";
            trans += transAttr;
            // 简单的颜色解析
            if (trans.length() == 7) {
                tileset->transparentColor.r = std::stoi(trans.substr(1, 2), nullptr, 16);
                tileset->transparentColor.g = std::stoi(trans.substr(3, 2), nullptr, 16);
                tileset->transparentColor.b = std::stoi(trans.substr(5, 2), nullptr, 16);
            }
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
            if (strcmp(orientation, "isometric") == 0) {
                tileset->grid.orientation = Orientation::Isometric;
            } else if (strcmp(orientation, "staggered") == 0) {
                tileset->grid.orientation = Orientation::Staggered;
            } else if (strcmp(orientation, "hexagonal") == 0) {
                tileset->grid.orientation = Orientation::Hexagonal;
            } else {
                tileset->grid.orientation = Orientation::Orthogonal;
            }
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
        const char* terrain = tileElem->Attribute("terrain");
        if (terrain) {
            // 解析地形信息 "0,0,0,0"
            std::string terrainStr = terrain;
            std::vector<std::string> parts;
            std::istringstream stream(terrainStr);
            std::string part;
            while (std::getline(stream, part, ',')) {
                parts.push_back(part);
            }
            if (parts.size() == 4) {
                tile.terrainTopLeft = !parts[0].empty() ? std::stoi(parts[0]) : -1;
                tile.terrainTopRight = !parts[1].empty() ? std::stoi(parts[1]) : -1;
                tile.terrainBottomLeft = !parts[2].empty() ? std::stoi(parts[2]) : -1;
                tile.terrainBottomRight = !parts[3].empty() ? std::stoi(parts[3]) : -1;
            }
        }

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
        tinyxml2::XMLElement* imgInTile = tileElem->FirstChildElement("image");
        if (imgInTile) {
            tile.imagePath = imgInTile->Attribute("source") ? imgInTile->Attribute("source") : "";
        }

        tileset->tiles[id] = tile;
    }

    // Wang sets
    for (tinyxml2::XMLElement* wangElem = tsElem->FirstChildElement("wangsets");
         wangElem != nullptr;
         wangElem = wangElem->NextSiblingElement("wangsets")) {

        for (tinyxml2::XMLElement* wangSetElem = wangElem->FirstChildElement("wangset");
             wangSetElem != nullptr;
             wangSetElem = wangSetElem->NextSiblingElement("wangset")) {

            WangSet wangSet;
            wangSet.name = wangSetElem->Attribute("name") ? wangSetElem->Attribute("name") : "";
            wangSet.tile = wangSetElem->IntAttribute("tile", -1);

            // Corner colors
            for (tinyxml2::XMLElement* colorElem = wangSetElem->FirstChildElement("wangcolor");
                 colorElem != nullptr;
                 colorElem = colorElem->NextSiblingElement("wangcolor")) {

                WangColor color;
                color.name = colorElem->Attribute("name") ? colorElem->Attribute("name") : "";
                color.color = colorElem->Attribute("color") ? colorElem->Attribute("color") : "";
                color.tile = colorElem->IntAttribute("tile", -1);
                color.probability = colorElem->FloatAttribute("probability", 1.0f);
                wangSet.cornerColors.push_back(color);
            }

            tileset->wangSets.push_back(wangSet);
        }
    }

    return tileset;
}

std::unique_ptr<Tileset> TsxParser::ParseString(const std::string& tsxContent) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.Parse(tsxContent.c_str());

    if (result != tinyxml2::XML_SUCCESS) {
        s_lastError = "Failed to parse TSX content";
        return nullptr;
    }

    tinyxml2::XMLElement* tsElem = doc.FirstChildElement("tileset");
    if (!tsElem) {
        s_lastError = "No tileset element found in TSX content";
        return nullptr;
    }

    auto tileset = std::make_unique<Tileset>();

    // 基本信息
    tileset->name = tsElem->Attribute("name") ? tsElem->Attribute("name") : "";
    tileset->tileWidth = tsElem->IntAttribute("tilewidth", 0);
    tileset->tileHeight = tsElem->IntAttribute("tileheight", 0);
    tileset->spacing = tsElem->IntAttribute("spacing", 0);
    tileset->margin = tsElem->IntAttribute("margin", 0);
    tileset->tileCount = tsElem->IntAttribute("tilecount", 0);
    tileset->columns = tsElem->IntAttribute("columns", 0);

    // 图像信息
    tinyxml2::XMLElement* imgElem = tsElem->FirstChildElement("image");
    if (imgElem) {
        tileset->imagePath = imgElem->Attribute("source") ? imgElem->Attribute("source") : "";
        tileset->imageWidth = imgElem->IntAttribute("width", 0);
        tileset->imageHeight = imgElem->IntAttribute("height", 0);
    }

    return tileset;
}

} // namespace PrismaEngine
