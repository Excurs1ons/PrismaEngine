#include "VoxelRenderer.h"
#include "RenderCommandContext.h"
#include "Logger.h"

namespace PrismaEngine {
namespace Graphic {

VoxelRenderer::VoxelRenderer() {
    LOG_INFO("VoxelRenderer", "创建 Voxel 渲染器");
}

VoxelRenderer::~VoxelRenderer() {
    m_chunks.clear();
    m_meshes.clear();
}

void VoxelRenderer::Initialize() {
    LOG_INFO("VoxelRenderer", "初始化 Voxel 渲染器");
}

void VoxelRenderer::Update(float deltaTime) {
    // 遍历所有区块，检查是否需要重建网格
    for (auto& [key, chunk] : m_chunks) {
        if (chunk->dirty) {
            RebuildMesh(key);
            chunk->dirty = false;
        }
    }
}

void VoxelRenderer::Render(RenderCommandContext* context) {
    if (!context) return;

    // 渲染所有已生成的区块网格
    for (auto& [key, meshInfo] : m_meshes) {
        if (meshInfo.visible && meshInfo.mesh) {
            // TODO: 设置材质并调用渲染命令
            // context->DrawMesh(meshInfo.mesh.get(), m_material.get());
        }
    }
}

void VoxelRenderer::AddChunk(int x, int y, int z, std::shared_ptr<VoxelChunk> chunk) {
    uint64_t key = GetKey(x, y, z);
    m_chunks[key] = chunk;
    LOG_DEBUG("VoxelRenderer", "添加区块: ({0}, {1}, {2})", x, y, z);
}

void VoxelRenderer::RemoveChunk(int x, int y, int z) {
    uint64_t key = GetKey(x, y, z);
    m_chunks.erase(key);
    m_meshes.erase(key);
}

void VoxelRenderer::RebuildMesh(uint64_t key) {
    auto it = m_chunks.find(key);
    if (it == m_chunks.end()) return;

    // TODO: 实现贪婪网格生成算法
    // 暂时创建一个空的 Mesh 占位
    LOG_DEBUG("VoxelRenderer", "正在重建区块网格: {0}", key);
    
    if (m_meshes.find(key) == m_meshes.end()) {
        m_meshes[key] = { std::make_unique<Mesh>(), true };
    }
}

} // namespace Graphic
} // namespace PrismaEngine
