#pragma once

#include "RenderComponent.h"
#include "Mesh.h"
#include "TextureAtlas.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Prisma {
namespace Graphic {

/**
 * @brief Voxel 区块数据
 * 定义 16x16x16 的体素网格
 */
struct VoxelChunk {
    static constexpr int SIZE = 16;
    static constexpr int VOLUME = SIZE * SIZE * SIZE;

    uint16_t blocks[VOLUME];
    bool dirty = true;

    VoxelChunk() {
        for (int i = 0; i < VOLUME; ++i) blocks[i] = 0;
    }

    void SetBlock(int x, int y, int z, uint16_t type) {
        if (x >= 0 && x < SIZE && y >= 0 && y < SIZE && z >= 0 && z < SIZE) {
            blocks[x + y * SIZE + z * SIZE * SIZE] = type;
            dirty = true;
        }
    }

    uint16_t GetBlock(int x, int y, int z) const {
        if (x >= 0 && x < SIZE && y >= 0 && y < SIZE && z >= 0 && z < SIZE) {
            return blocks[x + y * SIZE + z * SIZE * SIZE];
        }
        return 0;
    }
};

/**
 * @brief 高性能方块渲染器 (SDK 核心组件)
 * 采用异步网格烘焙和区块合并技术
 */
class VoxelRenderer : public RenderComponent {
public:
    VoxelRenderer();
    virtual ~VoxelRenderer();

    void Initialize() override;
    void Update(Timestep ts) override;
    void Render(RenderCommandContext* context) override;

    // 区块管理
    void AddChunk(int x, int y, int z, std::shared_ptr<VoxelChunk> chunk);
    void RemoveChunk(int x, int y, int z);

    // 纹理支持
    void SetTextureAtlas(std::shared_ptr<TextureAtlas> atlas) { m_atlas = atlas; }

private:
    struct ChunkMesh {
        std::unique_ptr<Mesh> mesh;
        bool visible = true;
    };

    std::unordered_map<uint64_t, std::shared_ptr<VoxelChunk>> m_chunks;
    std::unordered_map<uint64_t, ChunkMesh> m_meshes;
    std::shared_ptr<TextureAtlas> m_atlas;

    void RebuildMesh(uint64_t key);
    uint64_t GetKey(int x, int y, int z) const {
        return ((uint64_t)x << 42) | ((uint64_t)y << 21) | (uint64_t)z;
    }
};

} // namespace Graphic
} // namespace Prisma
