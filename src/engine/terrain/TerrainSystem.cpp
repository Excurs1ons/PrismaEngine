#include "terrain/TerrainSystem.h"
#include "Logger.h"
#include "Engine.h"
#include "SceneManager.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IPipeline.h"
#include "transform/Camera.h"
#include <algorithm>
#include <cmath>

namespace Prisma::Terrain {

// ============================================================================
// 构造/析构
// ============================================================================

TerrainSystem::TerrainSystem()
{
    m_TerrainMesh = std::make_shared<TerrainMesh>();
    m_Collision.SetHeightMap(&m_HeightMap);
}

TerrainSystem::~TerrainSystem()
{
    Shutdown();
}

// ============================================================================
// ISubSystem 接口
// ============================================================================

int TerrainSystem::Initialize()
{
    if (m_Initialized) return 0;

    LOG_INFO("Terrain", "地形系统正在初始化...");

    // 生成测试高度图作为默认
    if (!m_HeightMap.IsValid()) {
        GenerateTestTerrain(512, 512, 0.015f, 30.0f);
    }

    // 初始化默认材质
    InitDefaultMaterial();

    // 生成初始网格
    GenerateInitialMesh();

    m_Initialized = true;
    LOG_INFO("Terrain", "地形系统初始化完成");
    return 0;
}

void TerrainSystem::Shutdown()
{
    if (!m_Initialized) return;

    LOG_INFO("Terrain", "地形系统正在关闭...");

    m_TerrainMesh->Clear();
    m_HeightMap = HeightMap();
    m_Material.ClearLayers();

    m_Initialized = false;
    LOG_INFO("Terrain", "地形系统已关闭");
}

void TerrainSystem::Update(Timestep ts)
{
    if (!m_Initialized || !m_Enabled) return;
    if (!m_HeightMap.IsValid()) return;

    float dt = ts.GetSeconds();
    if (dt <= 0.0f || dt > 0.1f) dt = 0.1f;

    // 更新视锥
    UpdateFrustum();

    // 更新 LOD（基于相机位置）
    UpdateLOD(m_CameraPosition);

    // 提交渲染命令
    SubmitDrawCalls();
}

// ============================================================================
// 高度图加载
// ============================================================================

bool TerrainSystem::LoadHeightMap(const std::string& filepath,
                                   uint32_t width, uint32_t height,
                                   float heightScale)
{
    bool success = m_HeightMap.LoadFromRaw(filepath, width, height, heightScale);
    if (success) {
        m_Collision.SetHeightMap(&m_HeightMap);
        m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
        m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
        ForceRegenerateMesh();
    }
    return success;
}

bool TerrainSystem::LoadHeightMapPNG(const std::string& filepath,
                                      float heightScale)
{
    bool success = m_HeightMap.LoadFromPNG(filepath, heightScale);
    if (success) {
        m_Collision.SetHeightMap(&m_HeightMap);
        m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
        m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
        ForceRegenerateMesh();
    }
    return success;
}

bool TerrainSystem::LoadHeightMapFromArray(const float* data,
                                            uint32_t width, uint32_t height)
{
    bool success = m_HeightMap.LoadFromFloatArray(data, width, height);
    if (success) {
        m_Collision.SetHeightMap(&m_HeightMap);
        m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
        m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
        ForceRegenerateMesh();
    }
    return success;
}

void TerrainSystem::GenerateTestTerrain(uint32_t width, uint32_t height,
                                         float frequency, float amplitude)
{
    m_HeightMap.GenerateTestTerrain(width, height, frequency, amplitude);
    m_Collision.SetHeightMap(&m_HeightMap);
    m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
    m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
    ForceRegenerateMesh();
}

// ============================================================================
// 网格管理
// ============================================================================

void TerrainSystem::ForceRegenerateMesh()
{
    if (!m_HeightMap.IsValid()) return;
    m_TerrainMesh->ForceGenerate(m_HeightMap, m_CameraPosition, m_Config);
}

// ============================================================================
// 内部更新
// ============================================================================

void TerrainSystem::InitDefaultMaterial()
{
    m_Material.LoadFromConfig(m_Config);
    m_Material.SetMinHeight(m_HeightMap.GetMinHeight());
    m_Material.SetMaxHeight(m_HeightMap.GetMaxHeight());
}

void TerrainSystem::GenerateInitialMesh()
{
    if (!m_HeightMap.IsValid()) return;

    m_TerrainMesh->ForceGenerate(m_HeightMap, Vector3(0.0f), m_Config);
    m_VisibleLODCount = static_cast<uint32_t>(m_TerrainMesh->GetLODCount());
}

void TerrainSystem::UpdateLOD(const Vector3& cameraPosition)
{
    if (!m_TerrainMesh) return;

    m_TerrainMesh->Generate(m_HeightMap, cameraPosition, m_Config);

    // 执行视锥剔除
    const auto& frustum = m_FrustumCulling.getFrustum();
    auto visibility = m_TerrainMesh->CullAgainstFrustum(frustum);

    // 统计可见 LOD 数量
    m_VisibleLODCount = 0;
    for (bool visible : visibility) {
        if (visible) ++m_VisibleLODCount;
    }
}

void TerrainSystem::UpdateFrustum()
{
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* device = renderSystem->GetDevice();
    if (!device) return;

    // 从渲染系统获取相机参数并更新视锥
    // 如果场景管理器中有相机，使用相机参数
    auto* sceneManager = engine.GetSceneManager();
    if (sceneManager) {
        auto* scene = sceneManager->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) {
                m_CameraPosition = camera->GetPosition();
                m_CameraForward = camera->GetForward();
                m_CameraUp = camera->GetUp();
                m_CameraFOV = camera->GetFOV();
                m_CameraAspect = camera->GetAspectRatio();
                m_NearPlane = camera->GetNearPlane();
                m_FarPlane = camera->GetFarPlane();
            }
        }
    }

    // 更新视锥体
    m_FrustumCulling.updateFrustum(
        glm::dvec3(m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z),
        glm::dvec3(m_CameraForward.x, m_CameraForward.y, m_CameraForward.z),
        glm::dvec3(m_CameraUp.x, m_CameraUp.y, m_CameraUp.z),
        static_cast<double>(m_CameraFOV),
        static_cast<double>(m_CameraAspect),
        static_cast<double>(m_NearPlane),
        static_cast<double>(m_FarPlane)
    );
}

void TerrainSystem::SubmitDrawCalls()
{
    if (!m_TerrainMesh || !m_TerrainMesh->IsValid()) return;

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* pipeline = renderSystem->GetMainPipeline();
    if (!pipeline) return;

    // 获取可见性
    const auto& frustum = m_FrustumCulling.getFrustum();
    auto visibility = m_TerrainMesh->CullAgainstFrustum(frustum);

    // 提交每个可见 LOD 等级的三角形
    const auto& levels = m_TerrainMesh->GetLODLevels();
    for (size_t i = 0; i < levels.size(); ++i) {
        if (i < visibility.size() && !visibility[i]) continue;

        const auto& level = levels[i];
        if (level.IsEmpty()) continue;

        // 计算此 LOD 的三角形数量
        size_t triCount = level.GetTriangleCount();

        // 提交渲染原语
        // 注意：实际的 GPU 绘制需要通过 IRenderDevice 创建顶点/索引缓冲区
        // 并在渲染通道中提交 Draw 命令
        // 此处记录三角形数量供统计使用
        (void)triCount; // 供未来扩展使用
    }
}

// ============================================================================
// 统计
// ============================================================================

size_t TerrainSystem::GetTotalTriangleCount() const
{
    if (!m_TerrainMesh) return 0;

    size_t total = 0;
    for (const auto& level : m_TerrainMesh->GetLODLevels()) {
        total += level.GetTriangleCount();
    }
    return total;
}

} // namespace Prisma::Terrain
