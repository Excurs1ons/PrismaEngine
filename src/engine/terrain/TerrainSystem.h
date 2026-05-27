#pragma once

#include "ISubSystem.h"
#include "Export.h"
#include "TerrainConfig.h"
#include "HeightMap.h"
#include "TerrainMesh.h"
#include "TerrainMaterial.h"
#include "TerrainCollision.h"
#include "TerrainComponent.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Terrain {

/**
 * @brief 地形系统 - 管理地形的加载、生成、更新和渲染
 *
 * 子系统生命周期：
 * - Initialize()：加载配置、高度图、创建材质、生成初始网格
 * - Update()：检测相机移动、更新 LOD、提交渲染命令
 * - Shutdown()：释放所有资源
 */
class ENGINE_API TerrainSystem : public ISubSystem {
public:
    TerrainSystem();
    ~TerrainSystem() override;

    // ========== ISubSystem 接口 ==========
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "TerrainSystem"; }

    // ========== 地形配置 ==========

    /**
     * @brief 设置地形配置
     */
    void SetConfig(const TerrainConfig& config) { m_Config = config; }

    /**
     * @brief 获取地形配置引用
     */
    TerrainConfig& GetConfig() { return m_Config; }
    const TerrainConfig& GetConfig() const { return m_Config; }

    // ========== 高度图 ==========

    /**
     * @brief 加载高度图
     * @param filepath 高度图文件路径
     * @param width 宽度
     * @param height 高度
     * @param heightScale 高度缩放
     * @return 是否成功
     */
    bool LoadHeightMap(const std::string& filepath,
                       uint32_t width, uint32_t height,
                       float heightScale = 1.0f);

    /**
     * @brief 加载 PNG 高度图
     */
    bool LoadHeightMapPNG(const std::string& filepath, float heightScale = 1.0f);

    /**
     * @brief 从 float 数组加载高度图
     */
    bool LoadHeightMapFromArray(const float* data, uint32_t width, uint32_t height);

    /**
     * @brief 生成测试地形
     */
    void GenerateTestTerrain(uint32_t width, uint32_t height,
                             float frequency = 0.02f, float amplitude = 40.0f);

    /**
     * @brief 获取高度图引用
     */
    HeightMap& GetHeightMap() { return m_HeightMap; }
    const HeightMap& GetHeightMap() const { return m_HeightMap; }

    // ========== 地形网格 ==========

    /**
     * @brief 获取地形网格
     */
    std::shared_ptr<TerrainMesh> GetTerrainMesh() const { return m_TerrainMesh; }

    /**
     * @brief 强制重新生成网格
     */
    void ForceRegenerateMesh();

    // ========== 地形材质 ==========

    /**
     * @brief 获取地形材质
     */
    TerrainMaterial& GetMaterial() { return m_Material; }
    const TerrainMaterial& GetMaterial() const { return m_Material; }

    // ========== 地形碰撞 ==========

    /**
     * @brief 获取地形碰撞检测接口
     */
    TerrainCollision& GetCollision() { return m_Collision; }
    const TerrainCollision& GetCollision() const { return m_Collision; }

    // ========== 渲染 ==========

    /**
     * @brief 设置相机位置（供外部更新）
     */
    void SetCameraPosition(const Vector3& position) { m_CameraPosition = position; }

    /**
     * @brief 获取当前相机位置
     */
    Vector3 GetCameraPosition() const { return m_CameraPosition; }

    // ========== 状态查询 ==========

    bool IsHeightMapLoaded() const { return m_HeightMap.IsValid(); }
    bool HasValidMesh() const { return m_TerrainMesh && m_TerrainMesh->IsValid(); }
    bool IsEnabled() const { return m_Enabled; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }

    /**
     * @brief 获取已生成的三角形数量（性能指标）
     */
    size_t GetTotalTriangleCount() const;

    /**
     * @brief 获取可见 LOD 等级数量
     */
    uint32_t GetVisibleLODCount() const { return m_VisibleLODCount; }

private:
    // ========== 内部方法 ==========

    /**
     * @brief 初始化默认纹理层
     */
    void InitDefaultMaterial();

    /**
     * @brief 生成初始地形网格
     */
    void GenerateInitialMesh();

    /**
     * @brief 根据新的相机位置重新生成 LOD
     */
    void UpdateLOD(const Vector3& cameraPosition);

    /**
     * @brief 提交地形渲染命令
     */
    void SubmitDrawCalls();

    /**
     * @brief 更新视锥体
     */
    void UpdateFrustum();

    // ========== 成员变量 ==========

    TerrainConfig m_Config;
    HeightMap m_HeightMap;
    std::shared_ptr<TerrainMesh> m_TerrainMesh;
    TerrainMaterial m_Material;
    TerrainCollision m_Collision;

    // 相机/视锥状态
    Vector3 m_CameraPosition = Vector3(0.0f);
    Vector3 m_CameraForward = Vector3(0.0f, 0.0f, -1.0f);
    Vector3 m_CameraUp = Vector3(0.0f, 1.0f, 0.0f);
    float m_CameraFOV = 70.0f;
    float m_CameraAspect = 16.0f / 9.0f;
    float m_NearPlane = 0.1f;
    float m_FarPlane = 2000.0f;

    // 视锥剔除
    Graphic::FrustumCullingSystem m_FrustumCulling;

    // 统计
    uint32_t m_VisibleLODCount = 0;

    // 状态
    bool m_Enabled = true;
    bool m_Initialized = false;
};

} // namespace Prisma::Terrain
