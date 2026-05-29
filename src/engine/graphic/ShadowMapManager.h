#pragma once

#include "Export.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

// 阴影级联配置
/// 控制级联阴影映射的分割参数
struct ENGINE_API CascadeConfig {
    float splitLambda = 0.95f;      // 分割比例 (0~1, 对数空间 / 均匀空间混合)
    float nearPlane = 0.1f;         // 近平面
    float farPlane = 1000.0f;       // 远平面
    uint32_t cascadeCount = 4;      // 级联数量 (1~4)
};

// 阴影光源描述
/// 描述一个投射阴影的光源，用于阴影贴图生成和级联矩阵计算
struct ENGINE_API ShadowLight {
    PrismaMath::vec3 position = {0.0f, 0.0f, 0.0f};
    PrismaMath::vec3 direction = {0.0f, -1.0f, 0.0f};
    PrismaMath::vec3 color = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float shadowBias = 0.005f;          // 深度偏移
    float shadowNormalBias = 0.02f;     // 法线偏移
    bool castShadows = true;

    // 光源空间矩阵 (由 ShadowMapManager 计算)
    PrismaMath::mat4 lightView = PrismaMath::mat4(1.0f);
};

// 阴影贴图管理器
/// 管理阴影贴图纹理分配和级联矩阵计算
/// 支持定向光的级联阴影映射 (CSM)，每光源最多 4 个级联
class ENGINE_API ShadowMapManager {
public:
    // 最大级联数
    static constexpr uint32_t kMaxCascades = 4;
    // 默认阴影贴图分辨率
    static constexpr uint32_t kDefaultShadowMapSize = 2048;

    ShadowMapManager();
    ~ShadowMapManager();

    // === 初始化与清理 ===

    /// @brief 初始化管理器
    /// @param device 渲染设备
    /// @param factory 资源工厂 (用于创建纹理)
    /// @return 初始化是否成功
    bool Initialize(IRenderDevice* device, IResourceFactory* factory);

    /// @brief 释放所有纹理和矩阵资源
    void Cleanup();

    // === 每帧更新 ===

    /// @brief 更新级联分割和每光源的视投影矩阵
    /// @param cameraView 相机视图矩阵
    /// @param cameraProj 相机投影矩阵
    /// @param lights 阴影光源列表 (帧当前光源)
    void Update(const PrismaMath::mat4& cameraView,
                const PrismaMath::mat4& cameraProj,
                const std::vector<ShadowLight>& lights);

    // === 阴影贴图访问 ===

    /// @brief 获取指定光源和级联的阴影贴图纹理
    ITexture* GetShadowMap(uint32_t lightIndex, uint32_t cascadeIndex) const;

    /// @brief 获取指定光源的阴影贴图纹理数组
    ITexture* GetShadowMapArray(uint32_t lightIndex) const;

    /// @brief 获取指定光源和级联的视投影矩阵
    const PrismaMath::mat4& GetShadowMatrix(uint32_t lightIndex, uint32_t cascadeIndex) const;

    // === 级联配置 ===

    /// @brief 获取级联分割距离 (视空间 Z 值)
    const float* GetCascadeSplits() const { return m_cascadeSplits.data(); }

    /// @brief 获取级联数量
    uint32_t GetCascadeCount() const { return m_cascadeConfig.cascadeCount; }

    /// @brief 设置级联配置
    void SetCascadeConfig(const CascadeConfig& config) { m_cascadeConfig = config; }

    /// @brief 获取级联配置
    const CascadeConfig& GetCascadeConfig() const { return m_cascadeConfig; }

    // === 分辨率配置 ===

    /// @brief 设置阴影贴图分辨率 (宽高一致)
    void SetShadowMapSize(uint32_t size) { m_shadowMapSize = size; }

    /// @brief 获取阴影贴图分辨率
    uint32_t GetShadowMapSize() const { return m_shadowMapSize; }

    // === 光源管理 ===

    /// @brief 获取阴影光源数量
    uint32_t GetLightCount() const { return m_shadowLights.size(); }

    /// @brief 设置所有光源
    void SetLights(const std::vector<ShadowLight>& lights);

    /// @brief 添加一个光源
    void AddLight(const ShadowLight& light);

    /// @brief 清空所有光源
    void ClearLights();

    /// @brief 获取光源列表
    const std::vector<ShadowLight>& GetLights() const { return m_shadowLights; }

    /// @brief 修改单个光源
    void SetLight(uint32_t index, const ShadowLight& light);

    // === 状态查询 ===

    /// @brief 管理器是否已初始化
    bool IsValid() const { return m_initialized; }

private:
    // 创建光源的阴影贴图纹理 (Texture2DArray, D32_Float)
    bool CreateShadowTextures(uint32_t lightIndex);
    void DestroyShadowTextures(uint32_t lightIndex);
    void DestroyAllTextures();

    // 计算级联分割距离 (Practical Split Scheme)
    void ComputeCascadeSplits(const PrismaMath::mat4& cameraProj);

    // 计算指定光源的所有级联视投影矩阵
    void ComputeCascadeMatrices(uint32_t lightIndex,
                                const PrismaMath::mat4& cameraView,
                                const PrismaMath::mat4& cameraProj);

    // 设备/工厂指针
    IRenderDevice* m_device = nullptr;
    IResourceFactory* m_factory = nullptr;
    bool m_initialized = false;

    // 阴影贴图分辨率
    uint32_t m_shadowMapSize = kDefaultShadowMapSize;

    // 级联配置
    CascadeConfig m_cascadeConfig;

    // 阴影光源列表
    std::vector<ShadowLight> m_shadowLights;

    // 每光源的阴影贴图数组 (Texture2DArray, D32_Float)
    // m_shadowMapArrays[lightIndex] 包含该光源所有级联
    std::vector<std::shared_ptr<ITexture>> m_shadowMapArrays;

    // 每光源每级联的视投影矩阵
    // m_shadowMatrices[lightIndex * cascadeCount + cascadeIndex]
    std::vector<PrismaMath::mat4> m_shadowMatrices;

    // 级联分割距离 (视空间 Z 值)
    std::vector<float> m_cascadeSplits;
};

} // namespace Prisma::Graphic
