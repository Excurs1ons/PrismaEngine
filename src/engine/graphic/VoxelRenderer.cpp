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
            // 材质和渲染命令需要在渲染管线完善后实现
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

    // 贪婪网格生成算法需要在网格系统完善后实现
}

} // namespace Graphic
} // namespace PrismaEngine
