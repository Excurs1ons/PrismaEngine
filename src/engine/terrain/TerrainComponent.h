#pragma once

#include "core/Component.h"
#include "Export.h"
#include <memory>

namespace Prisma::Terrain {

// 前向声明
class TerrainMesh;

/**
 * @brief 地形组件 - ECS 组件，持有地形块引用
 *
 * 此组件附加到场景节点上，使该节点成为地形渲染节点。
 * 由 TerrainSystem 管理，提供地形块的世界变换和渲染数据。
 */
class ENGINE_API TerrainComponent : public Component {
public:
    TerrainComponent() = default;
    ~TerrainComponent() override = default;

    // ========== Component 接口 ==========
    void Initialize() override {}
    void Shutdown() override {}
    void Update(Timestep ts) override {}

    ComponentId GetComponentId() const override {
        return GetComponentTypeId<TerrainComponent>();
    }
    const char* GetComponentTypeName() const override {
        return "TerrainComponent";
    }

    // ========== 地形网格引用 ==========

    /**
     * @brief 设置关联的地形网格
     */
    void SetTerrainMesh(std::shared_ptr<TerrainMesh> mesh) { m_TerrainMesh = mesh; }

    /**
     * @brief 获取关联的地形网格
     */
    std::shared_ptr<TerrainMesh> GetTerrainMesh() const { return m_TerrainMesh; }

    /**
     * @brief 是否有有效的网格数据
     */
    bool HasMesh() const { return m_TerrainMesh != nullptr; }

private:
    std::shared_ptr<TerrainMesh> m_TerrainMesh;
};

} // namespace Prisma::Terrain
