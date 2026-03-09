#include "OBJParser.h"
#include "../Logger.h"
#include <format>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace PrismaEngine {
namespace Resource {

// 为了兼容旧代码，定义LOG_WARNING
#ifndef LOG_WARNING
#define LOG_WARNING(category, fmt, ...) ::Logger::GetInstance().LogFormat(::LogLevel::Warning, category, ::SourceLocation(__FILE__, __LINE__, __func__), fmt, ##__VA_ARGS__)
#endif

namespace PrismaEngine {
namespace Resource {

OBJParseResult OBJParser::Parse(const std::string& filePath) {
    OBJParseResult result;

    // 检查文件是否存在
    std::ifstream testFile(filePath);
    if (!testFile.good()) {
        result.error = "File does not exist: " + filePath;
        return result;
    }
    testFile.close();

    // 打开文件
    std::ifstream file(filePath);
    if (!file.is_open()) {
        result.error = "Failed to open file: " + filePath;
        return result;
    }

    std::string line;
    uint32_t currentMaterialIndex = 0;
    std::string currentGroupName = "default";

    OBJGroup currentGroup;
    currentGroup.name = currentGroupName;
    currentGroup.materialIndex = 0;

    try {
        while (std::getline(file, line)) {
            // 跳过空行和注释
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;

            // 根据关键字处理不同行类型
            if (keyword == "v") {
                // 顶点位置
                float x, y, z;
                if (iss >> x >> y >> z) {
                    result.positions.push_back(x);
                    result.positions.push_back(y);
                    result.positions.push_back(z);
                }
            }
            else if (keyword == "vt") {
                // 纹理坐标
                float u, v;
                if (iss >> u >> v) {
                    result.texCoords.push_back(u);
                    result.texCoords.push_back(v);
                }
            }
            else if (keyword == "vn") {
                // 法线
                float nx, ny, nz;
                if (iss >> nx >> ny >> nz) {
                    result.normals.push_back(nx);
                    result.normals.push_back(ny);
                    result.normals.push_back(nz);
                }
            }
            else if (keyword == "f") {
                // 面
                OBJFace face;
                std::string faceStr;
                while (iss >> faceStr) {
                    OBJFaceIndices indices;

                    // 解析面索引格式: v, v/vt, v/vt/vn, v//vn
                    size_t firstSlash = faceStr.find('/');
                    size_t secondSlash = faceStr.rfind('/');

                    if (firstSlash == std::string::npos) {
                        // 只有顶点索引
                        indices.vertexIndex = std::stoi(faceStr);
                    }
                    else if (firstSlash == secondSlash) {
                        // v/vt 格式
                        indices.vertexIndex = std::stoi(faceStr.substr(0, firstSlash));
                        indices.texCoordIndex = std::stoi(faceStr.substr(firstSlash + 1));
                    }
                    else {
                        // v/vt/vn 或 v//vn 格式
                        indices.vertexIndex = std::stoi(faceStr.substr(0, firstSlash));
                        std::string middle = faceStr.substr(firstSlash + 1, secondSlash - firstSlash - 1);
                        if (!middle.empty()) {
                            indices.texCoordIndex = std::stoi(middle);
                        }
                        indices.normalIndex = std::stoi(faceStr.substr(secondSlash + 1));
                    }

                    face.indices.push_back(indices);
                }

                currentGroup.faces.push_back(face);
            }
            else if (keyword == "g" || keyword == "o") {
                // 保存当前组
                if (!currentGroup.faces.empty()) {
                    result.groups.push_back(currentGroup);
                }

                // 开始新组
                iss >> currentGroupName;
                if (currentGroupName.empty()) {
                    currentGroupName = "group_" + std::to_string(result.groups.size());
                }
                currentGroup.name = currentGroupName;
                currentGroup.faces.clear();
            }
            else if (keyword == "usemtl") {
                // 使用材质
                std::string materialName;
                iss >> materialName;

                // 查找材质索引
                bool found = false;
                for (size_t i = 0; i < result.materials.size(); ++i) {
                    if (result.materials[i].name == materialName) {
                        currentMaterialIndex = static_cast<uint32_t>(i);
                        currentGroup.materialIndex = currentMaterialIndex;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    LOG_WARNING("OBJParser", "Material not found: {}", materialName);
                }
            }
            else if (keyword == "mtllib") {
                // 材质库文件
                std::string materialLib;
                iss >> materialLib;

                // 构建材质库文件路径
                std::filesystem::path basePath = std::filesystem::path(filePath).parent_path();
                std::filesystem::path materialPath = basePath / materialLib;

                // 读取材质文件
                if (!ReadMaterialFile(materialPath.string(), result.materials)) {
                    LOG_WARNING("OBJParser", "Failed to load material file: {}", materialPath.string());
                }
            }
        }

        // 保存最后一个组
        if (!currentGroup.faces.empty()) {
            result.groups.push_back(currentGroup);
        }

        // 检查是否至少有顶点和面
        if (result.positions.empty()) {
            result.error = "No vertices found in OBJ file";
            return result;
        }

        if (result.groups.empty() && result.faces.empty()) {
            result.error = "No faces found in OBJ file";
            return result;
        }

        // 如果没有组，将所有面放入一个默认组
        if (result.groups.empty()) {
            OBJGroup defaultGroup;
            defaultGroup.name = "default";
            defaultGroup.materialIndex = 0;
            // 添加所有面（之前已经添加到currentGroup了）
            defaultGroup.faces = currentGroup.faces;
            result.groups.push_back(defaultGroup);
        }

        result.success = true;

    } catch (const std::exception& e) {
        result.error = "Exception while parsing OBJ: " + std::string(e.what());
        result.success = false;
    }

    return result;
}

OBJParseResult OBJParser::ParseFromMemory(const char* data, size_t size) {
    OBJParseResult result;

    if (!data || size == 0) {
        result.error = "Invalid data or size";
        return result;
    }

    std::istringstream iss(std::string(data, size));
    std::string line;
    uint32_t currentMaterialIndex = 0;
    std::string currentGroupName = "default";

    OBJGroup currentGroup;
    currentGroup.name = currentGroupName;
    currentGroup.materialIndex = 0;

    try {
        while (std::getline(iss, line)) {
            // 跳过空行和注释
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream lineStream(line);
            std::string keyword;
            lineStream >> keyword;

            // 处理与Parse相同的逻辑
            if (keyword == "v") {
                float x, y, z;
                if (lineStream >> x >> y >> z) {
                    result.positions.push_back(x);
                    result.positions.push_back(y);
                    result.positions.push_back(z);
                }
            }
            else if (keyword == "vt") {
                float u, v;
                if (lineStream >> u >> v) {
                    result.texCoords.push_back(u);
                    result.texCoords.push_back(v);
                }
            }
            else if (keyword == "vn") {
                float nx, ny, nz;
                if (lineStream >> nx >> ny >> nz) {
                    result.normals.push_back(nx);
                    result.normals.push_back(ny);
                    result.normals.push_back(nz);
                }
            }
            else if (keyword == "f") {
                OBJFace face;
                std::string faceStr;
                while (lineStream >> faceStr) {
                    OBJFaceIndices indices;

                    size_t firstSlash = faceStr.find('/');
                    size_t secondSlash = faceStr.rfind('/');

                    if (firstSlash == std::string::npos) {
                        indices.vertexIndex = std::stoi(faceStr);
                    }
                    else if (firstSlash == secondSlash) {
                        indices.vertexIndex = std::stoi(faceStr.substr(0, firstSlash));
                        indices.texCoordIndex = std::stoi(faceStr.substr(firstSlash + 1));
                    }
                    else {
                        indices.vertexIndex = std::stoi(faceStr.substr(0, firstSlash));
                        std::string middle = faceStr.substr(firstSlash + 1, secondSlash - firstSlash - 1);
                        if (!middle.empty()) {
                            indices.texCoordIndex = std::stoi(middle);
                        }
                        indices.normalIndex = std::stoi(faceStr.substr(secondSlash + 1));
                    }

                    face.indices.push_back(indices);
                }

                currentGroup.faces.push_back(face);
            }
            else if (keyword == "g" || keyword == "o") {
                if (!currentGroup.faces.empty()) {
                    result.groups.push_back(currentGroup);
                }

                lineStream >> currentGroupName;
                if (currentGroupName.empty()) {
                    currentGroupName = "group_" + std::to_string(result.groups.size());
                }
                currentGroup.name = currentGroupName;
                currentGroup.faces.clear();
            }
            else if (keyword == "usemtl") {
                std::string materialName;
                lineStream >> materialName;

                for (size_t i = 0; i < result.materials.size(); ++i) {
                    if (result.materials[i].name == materialName) {
                        currentMaterialIndex = static_cast<uint32_t>(i);
                        currentGroup.materialIndex = currentMaterialIndex;
                        break;
                    }
                }
            }
        }

        if (!currentGroup.faces.empty()) {
            result.groups.push_back(currentGroup);
        }

        result.success = true;

    } catch (const std::exception& e) {
        result.error = "Exception while parsing OBJ from memory: " + std::string(e.what());
        result.success = false;
    }

    return result;
}

bool OBJParser::ReadVertexLine(std::istringstream& line, std::vector<float>& positions) {
    float x, y, z;
    if (line >> x >> y >> z) {
        positions.push_back(x);
        positions.push_back(y);
        positions.push_back(z);
        return true;
    }
    return false;
}

bool OBJParser::ReadTexCoordLine(std::istringstream& line, std::vector<float>& texCoords) {
    float u, v;
    if (line >> u >> v) {
        texCoords.push_back(u);
        texCoords.push_back(v);
        return true;
    }
    return false;
}

bool OBJParser::ReadNormalLine(std::istringstream& line, std::vector<float>& normals) {
    float nx, ny, nz;
    if (line >> nx >> ny >> nz) {
        normals.push_back(nx);
        normals.push_back(ny);
        normals.push_back(nz);
        return true;
    }
    return false;
}

bool OBJParser::ReadFaceLine(std::istringstream& line, OBJFace& face) {
    std::string faceStr;
    while (line >> faceStr) {
        OBJFaceIndices indices;

        size_t firstSlash = faceStr.find('/');
        size_t secondSlash = faceStr.rfind('/');

        if (firstSlash == std::string::npos) {
            indices.vertexIndex = std::stoi(faceStr);
        }
        else if (firstSlash == secondSlash) {
            indices.vertexIndex = std::stoi(faceStr.substr(0, firstSlash));
            indices.texCoordIndex = std::stoi(faceStr.substr(firstSlash + 1));
        }
        else {
            indices.vertexIndex = std::stoi(faceStr.substr(0, firstSlash));
            std::string middle = faceStr.substr(firstSlash + 1, secondSlash - firstSlash - 1);
            if (!middle.empty()) {
                indices.texCoordIndex = std::stoi(middle);
            }
            indices.normalIndex = std::stoi(faceStr.substr(secondSlash + 1));
        }

        face.indices.push_back(indices);
    }
    return !face.indices.empty();
}

bool OBJParser::ReadMaterialLibLine(std::istringstream& line, std::vector<std::string>& materialLibs) {
    std::string libName;
    if (line >> libName) {
        materialLibs.push_back(libName);
        return true;
    }
    return false;
}

bool OBJParser::ReadGroupLine(std::istringstream& line, std::string& groupName) {
    if (line >> groupName) {
        return true;
    }
    return false;
}

bool OBJParser::ReadUseMaterialLine(std::istringstream& line, uint32_t& materialIndex) {
    std::string materialName;
    if (line >> materialName) {
        // 注意：这个函数需要在调用者提供材质列表的上下文
        materialIndex = 0;
        return true;
    }
    return false;
}

bool OBJParser::ReadMaterialFile(const std::string& filePath, std::vector<OBJMaterial>& materials) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    OBJMaterial currentMaterial;
    bool hasMaterial = false;

    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "newmtl") {
            // 保存前一个材质
            if (hasMaterial) {
                materials.push_back(currentMaterial);
            }

            // 开始新材质
            currentMaterial = OBJMaterial();
            iss >> currentMaterial.name;
            hasMaterial = true;

            // 设置默认值
            currentMaterial.ambient[0] = 0.0f;
            currentMaterial.ambient[1] = 0.0f;
            currentMaterial.ambient[2] = 0.0f;
            currentMaterial.diffuse[0] = 1.0f;
            currentMaterial.diffuse[1] = 1.0f;
            currentMaterial.diffuse[2] = 1.0f;
            currentMaterial.specular[0] = 0.0f;
            currentMaterial.specular[1] = 0.0f;
            currentMaterial.specular[2] = 0.0f;
            currentMaterial.shininess = 0.0f;
            currentMaterial.opacity = 1.0f;
            currentMaterial.dissolve = 1.0f;
        }
        else if (keyword == "Ka") {
            // 环境光反射
            if (iss >> currentMaterial.ambient[0] >> currentMaterial.ambient[1] >> currentMaterial.ambient[2]) {
            }
        }
        else if (keyword == "Kd") {
            // 漫反射
            if (iss >> currentMaterial.diffuse[0] >> currentMaterial.diffuse[1] >> currentMaterial.diffuse[2]) {
            }
        }
        else if (keyword == "Ks") {
            // 镜面反射
            if (iss >> currentMaterial.specular[0] >> currentMaterial.specular[1] >> currentMaterial.specular[2]) {
            }
        }
        else if (keyword == "Ns") {
            // 光泽度
            iss >> currentMaterial.shininess;
        }
        else if (keyword == "d" || keyword == "Tr") {
            // 不透明度
            iss >> currentMaterial.opacity;
            currentMaterial.dissolve = currentMaterial.opacity;
        }
    }

    // 保存最后一个材质
    if (hasMaterial) {
        materials.push_back(currentMaterial);
    }

    return !materials.empty();
}

std::string OBJParser::ReadWord(std::istringstream& line) {
    std::string word;
    line >> word;
    return word;
}

bool OBJParser::SkipWhitespace(std::istringstream& line) {
    while (line.peek() == ' ' || line.peek() == '\t') {
        line.get();
    }
    return line.peek() != EOF && line.peek() != '\n' && line.peek() != '\r';
}

} // namespace Resource
} // namespace PrismaEngine
