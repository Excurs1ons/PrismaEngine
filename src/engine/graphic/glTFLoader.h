#pragma once

#include "Mesh.h"
#include <filesystem>

namespace Prisma::Graphic {

/**
 * @brief 从 glTF/GLB 文件加载网格数据
 *
 * 使用 cgltf 库解析 glTF 2.0 格式，支持：
 * - .gltf (JSON) 和 .glb (二进制) 格式
 * - 顶点属性: POSITION, NORMAL, TEXCOORD_0
 * - 8/16/32 位索引
 * - 多 mesh / 多 primitive
 * - PBR 材质索引映射
 *
 * @param mesh 目标 Mesh 对象（将填充 SubMeshBuffer + CPUMeshData）
 * @param path 文件路径
 * @return true 加载成功
 */
bool LoadGLTF(Mesh* mesh, const std::filesystem::path& path);

} // namespace Prisma::Graphic
