#pragma once

#include "../core/Tileset.h"
#include <string>
#include <memory>
#include <filesystem>

namespace PrismaEngine {

// ============================================================================
// TSX 解析器 (外部图块集文件)
// ============================================================================

class TsxParser {
public:
    // 解析 TSX 文件
    static std::unique_ptr<Tileset> ParseFile(const std::filesystem::path& filePath);

    // 解析 TSX 内容字符串
    static std::unique_ptr<Tileset> ParseString(const std::string& tsxContent);

    // 获取最后错误信息
    static const std::string& GetLastError() { return s_lastError; }

private:
    static std::string s_lastError;

    // 解析碰撞形状
    static std::vector<CollisionShape> ParseCollisionShapes(void* tileElement);

    // 解析属性
    static PropertyMap ParseProperties(void* propertiesElement);

    // 解析动画帧
    static std::vector<Frame> ParseAnimation(void* animationElement);
};

} // namespace PrismaEngine
