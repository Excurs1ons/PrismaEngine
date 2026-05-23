#include "Mesh.h"
#include "Logger.h"
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <tuple>

namespace Prisma::Graphic {

// glTF 加载器声明（实现在 glTFLoader.cpp）
bool LoadGLTF(Mesh* mesh, const std::filesystem::path& path);

// ── OBJ 加载器（保留作 glTF 的回退） ──
// 完整 glTF 加载器见 glTFLoader.h（通过 cgltf 实现）
static bool LoadOBJ(Mesh* mesh, const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    // 1. 第一遍：解析所有 v/vn/vt 数据
    std::vector<Prisma::Vector3> positions;
    std::vector<Prisma::Vector3> normals;
    std::vector<Prisma::Vector2> uvs;
    std::string line;
    {
        std::ifstream f(path);
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            std::string kw; iss >> kw;
            if (kw == "v") { float x,y,z; if (iss>>x>>y>>z) positions.emplace_back(x,y,z); }
            else if (kw == "vn") { float x,y,z; if (iss>>x>>y>>z) normals.emplace_back(x,y,z); }
            else if (kw == "vt") { float u,v; if (iss>>u>>v) uvs.emplace_back(u,v); }
        }
    }

    if (positions.empty()) return false;

    auto resolveIdx = [](int raw, size_t count) -> uint32_t {
        if (raw > 0) return uint32_t(raw - 1);
        if (raw < 0) return uint32_t(int(count) + raw);
        return UINT32_MAX;
    };

    // 2. 第二遍：解析面，对 (pos, norm, uv) 组合去重
    // 使用 map 以便快速查找已存在的顶点组合
    std::map<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t> vertMap;
    std::vector<Prisma::Vector3> finalPos;
    std::vector<Prisma::Vector3> finalNrm;
    std::vector<Prisma::Vector2> finalUv;
    std::vector<uint32_t> finalIdx;
    bool hasNormals = !normals.empty();

    auto getOrCreate = [&](uint32_t p, uint32_t n, uint32_t t) -> uint32_t {
        auto key = std::make_tuple(p, n, t);
        auto it = vertMap.find(key);
        if (it != vertMap.end()) return it->second;
        uint32_t idx = (uint32_t)finalPos.size();
        finalPos.push_back(positions[p]);
        finalNrm.push_back(hasNormals && n < normals.size() ? normals[n] : Prisma::Vector3(0,1,0));
        finalUv.push_back(t < uvs.size() ? uvs[t] : Prisma::Vector2(0));
        vertMap[key] = idx;
        return idx;
    };

    std::ifstream f2(path);
    while (std::getline(f2, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string kw; iss >> kw;
        if (kw != "f") continue;

        std::vector<uint32_t> faceVerts;
        std::string tok;
        while (iss >> tok) {
            // 解析 f v/vt/vn 或 v//vn 或 v
            size_t s1 = tok.find('/');
            size_t s2 = (s1 != std::string::npos) ? tok.find('/', s1 + 1) : std::string::npos;

            // 顶点位置索引（三种格式均存在）
            int vi = std::stoi(tok.substr(0, s1));
            uint32_t p = resolveIdx(vi, positions.size());
            if (p == UINT32_MAX) { faceVerts.clear(); break; }

            // 法线索引（v/vt/vn 或 v//vn）
            uint32_t n = 0;
            if (hasNormals && s2 != std::string::npos && s2 + 1 < tok.size()) {
                int vn = std::stoi(tok.substr(s2 + 1));
                n = resolveIdx(vn, normals.size());
                if (n == UINT32_MAX) n = 0;
            }

            // UV 索引（v/vt/vn 或 v/vt）
            uint32_t t = 0;
            if (s1 != std::string::npos) {
                // 存在第一个 /
                if (s2 == std::string::npos) {
                    // v/vt 格式：/ 之后是 UV，没有第二个 /
                    if (s1 + 1 < tok.size()) {
                        int vt = std::stoi(tok.substr(s1 + 1));
                        t = resolveIdx(vt, uvs.size());
                    }
                } else if (s2 > s1 + 1) {
                    // v/vt/vn 格式：第一个 / 之后、第二个 / 之前是 UV
                    int vt = std::stoi(tok.substr(s1 + 1, s2 - s1 - 1));
                    t = resolveIdx(vt, uvs.size());
                }
            }

            faceVerts.push_back(getOrCreate(p, n, t));
        }

        if (faceVerts.size() < 3) continue;
        for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
            finalIdx.push_back(faceVerts[0]);
            finalIdx.push_back(faceVerts[i]);
            finalIdx.push_back(faceVerts[i + 1]);
        }
    }

    if (finalPos.empty() || finalIdx.empty()) return false;

    // 3. 如果原始文件没有法线，从几何体计算平滑法线
    if (!hasNormals) {
        finalNrm.assign(finalPos.size(), Prisma::Vector3(0, 0, 0));
        for (size_t i = 0; i + 2 < finalIdx.size(); i += 3) {
            auto& p0 = finalPos[finalIdx[i]];
            auto& p1 = finalPos[finalIdx[i+1]];
            auto& p2 = finalPos[finalIdx[i+2]];
            glm::vec3 e1 = glm::vec3(p1) - glm::vec3(p0);
            glm::vec3 e2 = glm::vec3(p2) - glm::vec3(p0);
            glm::vec3 fn = glm::normalize(glm::cross(e1, e2));
            for (int j = 0; j < 3; j++)
                finalNrm[finalIdx[i+j]] = finalNrm[finalIdx[i+j]] + Prisma::Vector3(fn);
        }
        for (auto& n : finalNrm) {
            glm::vec3 gn(n);
            float len = glm::length(gn);
            n = (len > 1e-10f) ? Prisma::Vector3(gn / len) : Prisma::Vector3(0, 1, 0);
        }
    }

    // 4. 写入 Mesh
    BoundingBox bb;
    for (const auto& p : finalPos) bb.Encapsulate(p);
    mesh->SetBoundingBox(bb);

    SubMeshBuffer sub;
    sub.name = path.stem().string();
    sub.materialIndex = 0;
    sub.baseVertex = 0;
    sub.baseIndex = 0;
    sub.indexCount = static_cast<uint32_t>(finalIdx.size());
    sub.vertexCount = static_cast<uint32_t>(finalPos.size());
    sub.use16BitIndices = (finalPos.size() <= std::numeric_limits<uint16_t>::max());
    mesh->AddSubMesh(std::move(sub));

    Mesh::CPUMeshData cpu;
    cpu.positions = std::move(finalPos);
    cpu.normals   = std::move(finalNrm);
    cpu.uvs       = std::move(finalUv);
    cpu.indices   = std::move(finalIdx);
    mesh->AddCPUMeshData(std::move(cpu));

    LOG_INFO("Mesh", "OBJ 加载成功: {} (顶点={}, 三角面={})",
             path.filename().string(), finalPos.size(), finalIdx.size() / 3);
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
