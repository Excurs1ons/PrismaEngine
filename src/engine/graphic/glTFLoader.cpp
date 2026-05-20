#include "Mesh.h"
#include "Logger.h"
#include <filesystem>
#include <limits>
#include <cstdint>

// cgltf — 单文件 glTF 2.0 加载器 (https://github.com/jkuhlmann/cgltf)
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace Prisma::Graphic {

/// 从 cgltf_accessor 解包数据到 vector
template<typename T>
static bool UnpackAccessor(const cgltf_accessor* accessor, std::vector<T>& out)
{
    if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer) {
        return false;
    }
    cgltf_size count = accessor->count;
    out.resize(count);
    for (cgltf_size i = 0; i < count; ++i) {
        cgltf_float value[4] = {0, 0, 0, 1};
        cgltf_accessor_read_float(accessor, i, value, 4);
        if constexpr (std::is_same_v<T, ::Prisma::Vector3>) {
            out[i] = ::Prisma::Vector3(value[0], value[1], value[2]);
        } else if constexpr (std::is_same_v<T, ::Prisma::Vector2>) {
            out[i] = ::Prisma::Vector2(value[0], value[1]);
        }
    }
    return true;
}

static bool UnpackIndices(const cgltf_accessor* accessor, std::vector<uint32_t>& out)
{
    if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer) {
        return false;
    }
    cgltf_size count = accessor->count;
    out.resize(count);

    const uint8_t* src = static_cast<const uint8_t*>(accessor->buffer_view->buffer->data)
                         + accessor->offset + accessor->buffer_view->offset;

    for (cgltf_size i = 0; i < count; ++i) {
        switch (accessor->component_type) {
        case cgltf_component_type_r_8u:
            out[i] = static_cast<uint32_t>(src[i]); break;
        case cgltf_component_type_r_16u:
            out[i] = static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(src)[i]); break;
        case cgltf_component_type_r_32u:
        default:
            out[i] = reinterpret_cast<const uint32_t*>(src)[i]; break;
        }
    }
    return true;
}

bool LoadGLTF(Mesh* mesh, const std::filesystem::path& path)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path.string().c_str(), &data);
    if (result != cgltf_result_success) {
        LOG_ERROR("glTF", "解析失败: {}", path.string());
        return false;
    }

    result = cgltf_load_buffers(&options, data, path.string().c_str());
    if (result != cgltf_result_success) {
        LOG_ERROR("glTF", "缓冲区加载失败: {}", path.string());
        cgltf_free(data);
        return false;
    }

    // 计算全局包围盒
    ::Prisma::Vector3 bbMin(std::numeric_limits<float>::max());
    ::Prisma::Vector3 bbMax(std::numeric_limits<float>::lowest());
    BoundingBox globalBB(bbMin, bbMax);

    // 遍历所有 mesh
    for (cgltf_size m = 0; m < data->meshes_count; ++m) {
        const auto& srcMesh = data->meshes[m];
        std::string meshName = srcMesh.name ? srcMesh.name : path.stem().string();

        for (cgltf_size p = 0; p < srcMesh.primitives_count; ++p) {
            const auto& prim = srcMesh.primitives[p];

            // ── 读取顶点属性 ──
            std::vector<::Prisma::Vector3> positions;
            std::vector<::Prisma::Vector3> normals;
            std::vector<::Prisma::Vector2> uvs;

            for (cgltf_size a = 0; a < prim.attributes_count; ++a) {
                const auto& attr = prim.attributes[a];

                if (attr.type == cgltf_attribute_type_position) {
                    UnpackAccessor(attr.data, positions);
                } else if (attr.type == cgltf_attribute_type_normal) {
                    UnpackAccessor(attr.data, normals);
                } else if (attr.type == cgltf_attribute_type_texcoord && attr.index == 0) {
                    UnpackAccessor(attr.data, uvs);
                }
            }

            if (positions.empty()) {
                LOG_WARN("glTF", "网格 '{}' primitive {} 缺少 POSITION 属性", meshName, p);
                continue;
            }

            // ── 读取索引 ──
            std::vector<uint32_t> indices;
            if (prim.indices) {
                UnpackIndices(prim.indices, indices);
            } else {
                // 非索引网格：生成 0,1,2,3,...
                indices.resize(positions.size());
                for (size_t i = 0; i < positions.size(); ++i)
                    indices[i] = static_cast<uint32_t>(i);
            }

            // ── 更新包围盒 ──
            for (const auto& pos : positions) {
                globalBB.Encapsulate(pos);
            }

            // ── 创建 SubMeshBuffer ──
            SubMeshBuffer sub;
            sub.name = meshName + "_" + std::to_string(p);
            sub.materialIndex = prim.material ? static_cast<uint32_t>(prim.material - data->materials) : 0;
            sub.baseVertex = 0;
            sub.baseIndex = 0;
            sub.indexCount = static_cast<uint32_t>(indices.size());
            sub.vertexCount = static_cast<uint32_t>(positions.size());
            sub.use16BitIndices = (positions.size() <= std::numeric_limits<uint16_t>::max());
            mesh->AddSubMesh(std::move(sub));

            // ── 存储 CPU 数据 ──
            Mesh::CPUMeshData cpu;
            cpu.positions = std::move(positions);
            cpu.normals   = std::move(normals);
            cpu.uvs       = std::move(uvs);
            cpu.indices   = std::move(indices);
            mesh->AddCPUMeshData(std::move(cpu));
        }
    }

    mesh->SetBoundingBox(globalBB);

    LOG_INFO("glTF", "加载成功: {} ({} meshes, {} primitives)",
             path.filename().string(), data->meshes_count, mesh->GetCPUSubMeshes().size());

    cgltf_free(data);
    return true;
}

} // namespace Prisma::Graphic
