#include "Mesh.h"
#include "Logger.h"
#include <fstream>
#include <limits>
#include <sstream>

namespace Prisma::Graphic {

// glTF 加载器声明（实现在 glTFLoader.cpp）
bool LoadGLTF(Mesh* mesh, const std::filesystem::path& path);

// ── OBJ 加载器（保留作 glTF 的回退） ──
// 完整 glTF 加载器见 glTFLoader.h（通过 cgltf 实现）
static bool LoadOBJ(Mesh* mesh, const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::vector<Prisma::Vector3> positions;
    std::vector<Prisma::Vector3> normals;
    std::vector<Prisma::Vector2> uvs;
    std::vector<uint32_t> indices;
    std::string line;

    auto resolveIndex = [](int rawIndex, size_t count) -> uint32_t {
        if (rawIndex > 0) return static_cast<uint32_t>(rawIndex - 1);
        if (rawIndex < 0) return static_cast<uint32_t>(static_cast<int>(count) + rawIndex);
        return std::numeric_limits<uint32_t>::max();
    };

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "v") {
            float x = 0, y = 0, z = 0;
            if (iss >> x >> y >> z)
                positions.emplace_back(x, y, z);
            continue;
        }
        if (keyword == "vn") {
            float x = 0, y = 0, z = 0;
            if (iss >> x >> y >> z)
                normals.emplace_back(x, y, z);
            continue;
        }
        if (keyword == "vt") {
            float u = 0, v = 0;
            if (iss >> u >> v)
                uvs.emplace_back(u, v);
            continue;
        }
        if (keyword != "f") continue;

        std::vector<uint32_t> faceIndices;
        std::string token;
        while (iss >> token) {
            // 解析 v/vt/vn 或 v//vn 或 v
            size_t s1 = token.find('/');
            size_t s2 = (s1 != std::string::npos) ? token.find('/', s1 + 1) : std::string::npos;

            int vIdx = std::stoi(token.substr(0, s1));
            uint32_t resolved = resolveIndex(vIdx, positions.size());
            if (resolved == std::numeric_limits<uint32_t>::max() || resolved >= positions.size()) {
                faceIndices.clear();
                break;
            }
            faceIndices.push_back(resolved);
        }

        if (faceIndices.size() < 3) continue;

        for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[i]);
            indices.push_back(faceIndices[i + 1]);
        }
    }

    if (positions.empty() || indices.empty()) return false;

    // 写入 Mesh
    constexpr float kMaxVal = (std::numeric_limits<float>::max)();
    constexpr float kLowVal = (std::numeric_limits<float>::lowest)();
    BoundingBox bb{Prisma::Vector3(kMaxVal), Prisma::Vector3(kLowVal)};
    for (const auto& p : positions) bb.Encapsulate(p);
    mesh->SetBoundingBox(bb);

    SubMeshBuffer sub;
    sub.name = path.stem().string();
    sub.materialIndex = 0;
    sub.baseVertex = 0;
    sub.baseIndex = 0;
    sub.indexCount = static_cast<uint32_t>(indices.size());
    sub.vertexCount = static_cast<uint32_t>(positions.size());
    sub.use16BitIndices = (positions.size() <= std::numeric_limits<uint16_t>::max());
    mesh->AddSubMesh(std::move(sub));

    // 存储 CPU 数据
    Mesh::CPUMeshData cpu;
    cpu.positions = std::move(positions);
    cpu.normals   = std::move(normals);
    cpu.uvs       = std::move(uvs);
    cpu.indices   = std::move(indices);
    mesh->AddCPUMeshData(std::move(cpu));

    return true;
}

bool Mesh::Load(const std::filesystem::path& path)
{
    m_SubMeshes.clear();
    m_cpuData.clear();
    m_BoundingBox = BoundingBox();

    auto ext = path.extension().string();
    // 转小写
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    bool ok = false;
    if (ext == ".gltf" || ext == ".glb") {
        ok = LoadGLTF(this, path);
    } else if (ext == ".obj") {
        ok = LoadOBJ(this, path);
    } else {
        LOG_ERROR("Mesh", "不支持的网格格式: {0}", path.string());
        ok = false;
    }

    if (!ok) {
        LOG_ERROR("Mesh", "网格加载失败: {0}", path.string());
        m_SubMeshes.clear();
        m_cpuData.clear();
        return false;
    }

    SetPath(path);
    SetName(path.stem().string());
    m_IsLoaded = true;
    LOG_INFO("Mesh", "加载成功: {} (顶点={}, 三角面={})",
             path.filename().string(),
             m_cpuData.empty() ? 0 : m_cpuData[0].positions.size(),
             m_cpuData.empty() ? 0 : m_cpuData[0].indices.size() / 3);
    return true;
}

void Mesh::Unload()
{
    m_SubMeshes.clear();
    m_cpuData.clear();
    m_BoundingBox = BoundingBox();
    m_IsLoaded = false;
}

} // namespace Prisma::Graphic
