#pragma once

#include "Export.h"
#include "Tilemap.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IShader.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {
    class IRenderDevice;
    class ICommandBuffer;
}

namespace Prisma::Tilemap {

struct TilemapCameraUBO {
    Matrix4 viewProjection;
};
static_assert(sizeof(TilemapCameraUBO) == 64, "TilemapCameraUBO must be 64 bytes (std140)");

struct TileInstanceData {
    Matrix4 world;
    Vector4 uv;
};
static_assert(sizeof(TileInstanceData) == 80, "TileInstanceData must be 80 bytes");

constexpr uint32_t kMaxTileInstances = 65536;

class ENGINE_API TilemapRenderer {
public:
    TilemapRenderer() = default;
    ~TilemapRenderer();

    void SetTilemap(std::shared_ptr<Tilemap> tilemap) { m_tilemap = std::move(tilemap); }
    std::shared_ptr<Tilemap> GetTilemap() const { return m_tilemap; }

    void Render(const Graphic::OrthographicCamera& camera);

    void SetTexture(std::shared_ptr<Graphic::ITexture> texture) { m_texture = std::move(texture); }

    void SetUseInstancing(bool enable) { m_useInstancing = enable; }
    bool IsUsingInstancing() const { return m_useInstancing; }

private:
    void EnsureInstancingResources(Graphic::IRenderDevice* device);
    void BuildInstanceList(const Graphic::OrthographicCamera& camera);
    void RenderInstanced(Graphic::ICommandBuffer* cmd);

    std::shared_ptr<Tilemap> m_tilemap;
    std::shared_ptr<Graphic::ITexture> m_texture;
    bool m_useInstancing = true;

    std::shared_ptr<Graphic::IBuffer> m_quadVertexBuffer;
    std::shared_ptr<Graphic::IBuffer> m_quadIndexBuffer;
    std::shared_ptr<Graphic::IBuffer> m_instanceBuffer;
    std::shared_ptr<Graphic::IBuffer> m_cameraUBO;

    std::shared_ptr<Graphic::IPipelineState> m_instancedPSO;
    std::shared_ptr<Graphic::IShader> m_instancedVertShader;
    std::shared_ptr<Graphic::IShader> m_instancedFragShader;
    std::shared_ptr<Graphic::IDescriptorSet> m_instancedDescSet;
    std::shared_ptr<Graphic::IDescriptorSetLayout> m_instancedDescSetLayout;

    std::vector<Matrix4> m_instanceMatrices;
    std::vector<Vector4> m_instanceUVs;
    uint32_t m_instanceCount = 0;
};

}
