#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <unordered_map>
#include <cstdint>

namespace PrismaEngine {
namespace Resource {

/**
 * @brief OBJ纹理坐标数据结构
 */
struct OBJTexCoord {
    float u, v;
};

/**
 * @brief OBJ面索引数据结构
 */
struct OBJFaceIndices {
    uint32_t vertexIndex;
    uint32_t texCoordIndex;
    uint32_t normalIndex;
};

/**
 * @brief OBJ面数据结构
 */
struct OBJFace {
    std::vector<OBJFaceIndices> indices;
};

/**
 * @brief OBJ组数据结构
 */
struct OBJGroup {
    std::string name;
    std::vector<OBJFace> faces;
    uint32_t materialIndex;
};

/**
 * @brief OBJ材质数据结构
 */
struct OBJMaterial {
    std::string name;
    float ambient[3];
    float diffuse[3];
    float specular[3];
    float shininess;
    float opacity;
    float dissolve;
};

/**
 * @brief OBJ文件解析结果
 */
struct OBJParseResult {
    bool success;
    std::string error;

    // 顶点数据（来自OBJ文件）
    std::vector<float> positions;  // 位置数据 (x, y, z)
    std::vector<float> texCoords;  // 纹理坐标 (u, v)
    std::vector<float> normals;    // 法线数据 (nx, ny, nz)

    // 面数据
    std::vector<OBJFace> faces;

    // 材质
    std::vector<OBJMaterial> materials;

    // 组
    std::vector<OBJGroup> groups;

    OBJParseResult() : success(false) {}
};

/**
 * @brief OBJ文件解析器
 */
class OBJParser {
public:
    /**
     * @brief 解析OBJ文件
     * @param filePath OBJ文件路径
     * @return 解析结果
     */
    static OBJParseResult Parse(const std::string& filePath);

    /**
     * @brief 从内存解析OBJ数据
     * @param data OBJ文件数据
     * @param size 数据大小
     * @return 解析结果
     */
    static OBJParseResult ParseFromMemory(const char* data, size_t size);

private:
    // 读取各种行类型
    static bool ReadVertexLine(std::istringstream& line, std::vector<float>& positions);
    static bool ReadTexCoordLine(std::istringstream& line, std::vector<float>& texCoords);
    static bool ReadNormalLine(std::istringstream& line, std::vector<float>& normals);
    static bool ReadFaceLine(std::istringstream& line, OBJFace& face);
    static bool ReadMaterialLibLine(std::istringstream& line, std::vector<std::string>& materialLibs);
    static bool ReadGroupLine(std::istringstream& line, std::string& groupName);
    static bool ReadUseMaterialLine(std::istringstream& line, uint32_t& materialIndex);

    // 读取材质文件
    static bool ReadMaterialFile(const std::string& filePath, std::vector<OBJMaterial>& materials);

    // 辅助函数
    static std::string ReadWord(std::istringstream& line);
    static bool SkipWhitespace(std::istringstream& line);
};

} // namespace Resource
} // namespace PrismaEngine
