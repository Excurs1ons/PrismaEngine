#include "graphic/ShadowMapManager.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include <algorithm>
#include <cmath>
#include <array>

namespace Prisma::Graphic {

// ============================================================================
// 工具函数
// ============================================================================

namespace {

// 从投影矩阵提取近/远平面
void ExtractNearFarFromProj(const PrismaMath::mat4& proj, float& nearPlane, float& farPlane) {
    // 透视投影: proj[2][2] = (far+near)/(near-far), proj[3][2] = 2*far*near/(near-far)
    // 正交投影: proj[2][2] = 2/(near-far), proj[3][2] = (near+far)/(near-far)
    if (proj[3][3] < 0.001f) { // 透视投影
        float a = proj[2][2];
        float b = proj[3][2];
        nearPlane = b / (a - 1.0f);
        farPlane = b / (a + 1.0f);
    } else { // 正交投影
        nearPlane = (proj[3][2] + 1.0f) / proj[2][2];
        farPlane = (proj[3][2] - 1.0f) / proj[2][2];
    }
    if (nearPlane > farPlane) std::swap(nearPlane, farPlane);
}

// 计算级联分割距离 (Practical Split Scheme)
void ComputePracticalSplits(uint32_t cascadeCount, float nearPlane, float farPlane,
                            float lambda, float* splits) {
    splits[0] = nearPlane;
    for (uint32_t i = 1; i < cascadeCount; ++i) {
        float fraction = static_cast<float>(i) / static_cast<float>(cascadeCount);
        float logSplit = nearPlane * std::pow(farPlane / nearPlane, fraction);
        float uniformSplit = nearPlane + (farPlane - nearPlane) * fraction;
        splits[i] = logSplit * lambda + uniformSplit * (1.0f - lambda);
    }
    splits[cascadeCount] = farPlane;
}

// 计算视锥体在光照空间中的 AABB，返回中心点和范围
void ComputeLightSpaceAABB(const PrismaMath::vec3 frustumCorners[8],
                           const PrismaMath::mat4& lightView,
                           PrismaMath::vec3& center,
                           PrismaMath::vec3& extents) {
    PrismaMath::vec3 minBounds(1e30f), maxBounds(-1e30f);

    for (int i = 0; i < 8; ++i) {
        PrismaMath::vec4 cornerLS = lightView * PrismaMath::vec4(frustumCorners[i], 1.0f);
        minBounds = PrismaMath::min(minBounds, PrismaMath::vec3(cornerLS));
        maxBounds = PrismaMath::max(maxBounds, PrismaMath::vec3(cornerLS));
    }

    center = (minBounds + maxBounds) * 0.5f;
    extents = (maxBounds - minBounds) * 0.5f;
}

// 创建正交投影矩阵（光照空间）
PrismaMath::mat4 MakeOrthoMatrix(float left, float right, float bottom, float top,
                                  float nearZ, float farZ) {
    PrismaMath::mat4 result(1.0f);
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = 1.0f / (farZ - nearZ);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -nearZ / (farZ - nearZ);
    return result;
}

// 创建光照空间视图矩阵 (LookAt)
PrismaMath::mat4 MakeLightViewMatrix(const PrismaMath::vec3& position,
                                      const PrismaMath::vec3& direction,
                                      const PrismaMath::vec3& up) {
    PrismaMath::vec3 f = PrismaMath::normalize(direction);
    PrismaMath::vec3 s = PrismaMath::normalize(PrismaMath::cross(f, up));
    PrismaMath::vec3 u = PrismaMath::cross(s, f);

    PrismaMath::mat4 result(1.0f);
    result[0][0] = s.x; result[0][1] = u.x; result[0][2] = -f.x; result[0][3] = 0.0f;
    result[1][0] = s.y; result[1][1] = u.y; result[1][2] = -f.y; result[1][3] = 0.0f;
    result[2][0] = s.z; result[2][1] = u.z; result[2][2] = -f.z; result[2][3] = 0.0f;
    result[3][0] = -PrismaMath::dot(s, position);
    result[3][1] = -PrismaMath::dot(u, position);
    result[3][2] =  PrismaMath::dot(f, position);
    result[3][3] = 1.0f;
    return result;
}

// 构建视锥体角点 (视空间)
void BuildFrustumCorners(float nearZ, float farZ, float fovY, float aspectRatio,
                         PrismaMath::vec3 corners[8]) {
    float tanHalfFov = std::tan(fovY * 0.5f);
    float nearH = nearZ * tanHalfFov;
    float nearW = nearH * aspectRatio;
    float farH = farZ * tanHalfFov;
    float farW = farH * aspectRatio;

    // 近平面 4 点
    corners[0] = PrismaMath::vec3(-nearW, -nearH, nearZ);
    corners[1] = PrismaMath::vec3( nearW, -nearH, nearZ);
    corners[2] = PrismaMath::vec3( nearW,  nearH, nearZ);
    corners[3] = PrismaMath::vec3(-nearW,  nearH, nearZ);
    // 远平面 4 点
    corners[4] = PrismaMath::vec3(-farW, -farH, farZ);
    corners[5] = PrismaMath::vec3( farW, -farH, farZ);
    corners[6] = PrismaMath::vec3( farW,  farH, farZ);
    corners[7] = PrismaMath::vec3(-farW,  farH, farZ);
}

} // anonymous namespace

// ============================================================================
// ShadowMapManager
// ============================================================================

ShadowMapManager::ShadowMapManager()
    : m_device(nullptr)
    , m_factory(nullptr)
    , m_initialized(false)
    , m_shadowMapSize(kDefaultShadowMapSize)
    , m_cascadeSplits(kMaxCascades + 1, 0.0f) {
}

ShadowMapManager::~ShadowMapManager() {
    Cleanup();
}

bool ShadowMapManager::Initialize(IRenderDevice* device, IResourceFactory* factory) {
    if (!device || !factory) {
        return false;
    }

    m_device = device;
    m_factory = factory;
    m_initialized = true;

    // 预分配级联分割数组
    m_cascadeSplits.resize(m_cascadeConfig.cascadeCount + 1, 0.0f);

    return true;
}

void ShadowMapManager::Cleanup() {
    DestroyAllTextures();
    m_shadowMatrices.clear();
    m_cascadeSplits.clear();
    m_shadowLights.clear();
    m_initialized = false;
    m_device = nullptr;
    m_factory = nullptr;
}

void ShadowMapManager::Update(const PrismaMath::mat4& cameraView,
                               const PrismaMath::mat4& cameraProj,
                               const std::vector<ShadowLight>& lights) {
    if (!m_initialized) return;

    // 更新光源列表
    m_shadowLights = lights;

    // 提取近/远平面
    float nearPlane, farPlane;
    ExtractNearFarFromProj(cameraProj, nearPlane, farPlane);

    // 更新级联配置中的近/远平面
    m_cascadeConfig.nearPlane = nearPlane;
    m_cascadeConfig.farPlane = farPlane;

    // 计算级联分割
    ComputeCascadeSplits(cameraProj);

    // 确保矩阵存储足够大
    uint32_t totalMatrices = static_cast<uint32_t>(m_shadowLights.size()) * m_cascadeConfig.cascadeCount;
    if (m_shadowMatrices.size() < totalMatrices) {
        m_shadowMatrices.resize(totalMatrices);
    }

    // 为每个光源更新阴影贴图纹理和矩阵
    for (uint32_t lightIdx = 0; lightIdx < m_shadowLights.size(); ++lightIdx) {
        auto& light = m_shadowLights[lightIdx];
        if (!light.castShadows) continue;

        // 计算光源视图矩阵
        PrismaMath::vec3 up(0.0f, 1.0f, 0.0f);
        // 如果光方向平行于上向量，调整上向量
        if (std::abs(PrismaMath::dot(PrismaMath::normalize(light.direction), up)) > 0.99f) {
            up = PrismaMath::vec3(1.0f, 0.0f, 0.0f);
        }
        light.lightView = MakeLightViewMatrix(light.position, light.direction, up);

        // 确保纹理存在
        if (lightIdx >= m_shadowMapArrays.size() || !m_shadowMapArrays[lightIdx]) {
            CreateShadowTextures(lightIdx);
        }

        // 计算每级联矩阵
        ComputeCascadeMatrices(lightIdx, cameraView);
    }
}

ITexture* ShadowMapManager::GetShadowMap(uint32_t lightIndex, uint32_t cascadeIndex) const {
    if (lightIndex >= m_shadowMapArrays.size() || !m_shadowMapArrays[lightIndex]) {
        return nullptr;
    }
    // 从 Texture2DArray 中获取单个切片
    // 注意: 返回的是整个纹理数组，由着色器通过 arrayIndex 访问
    // 这里返回纹理数组，调用方使用 cascadeIndex 作为数组索引
    return m_shadowMapArrays[lightIndex].get();
}

ITexture* ShadowMapManager::GetShadowMapArray(uint32_t lightIndex) const {
    if (lightIndex >= m_shadowMapArrays.size()) {
        return nullptr;
    }
    return m_shadowMapArrays[lightIndex].get();
}

const PrismaMath::mat4& ShadowMapManager::GetShadowMatrix(uint32_t lightIndex,
                                                           uint32_t cascadeIndex) const {
    uint32_t idx = lightIndex * m_cascadeConfig.cascadeCount + cascadeIndex;
    // 边界检查 - 如果越界返回单位矩阵 (静态)
    static const PrismaMath::mat4 identity(1.0f);
    if (idx >= m_shadowMatrices.size()) {
        return identity;
    }
    return m_shadowMatrices[idx];
}

void ShadowMapManager::SetLights(const std::vector<ShadowLight>& lights) {
    m_shadowLights = lights;

    // 确保纹理数量匹配
    for (uint32_t i = 0; i < m_shadowLights.size(); ++i) {
        if (m_shadowLights[i].castShadows) {
            if (i >= m_shadowMapArrays.size() || !m_shadowMapArrays[i]) {
                CreateShadowTextures(i);
            }
        }
    }
}

void ShadowMapManager::AddLight(const ShadowLight& light) {
    m_shadowLights.push_back(light);
    if (light.castShadows) {
        CreateShadowTextures(static_cast<uint32_t>(m_shadowLights.size()) - 1);
    }
}

void ShadowMapManager::ClearLights() {
    m_shadowLights.clear();
}

void ShadowMapManager::SetLight(uint32_t index, const ShadowLight& light) {
    if (index < m_shadowLights.size()) {
        m_shadowLights[index] = light;
        if (light.castShadows && (index >= m_shadowMapArrays.size() || !m_shadowMapArrays[index])) {
            CreateShadowTextures(index);
        }
    }
}

// ============================================================================
// 私有方法
// ============================================================================

bool ShadowMapManager::CreateShadowTextures(uint32_t lightIndex) {
    if (!m_factory) return false;

    // 确保容器大小
    while (m_shadowMapArrays.size() <= lightIndex) {
        m_shadowMapArrays.push_back(nullptr);
    }

    uint32_t cascadeCount = m_cascadeConfig.cascadeCount;

    TextureDesc desc;
    desc.type = TextureType::Texture2DArray;
    desc.format = TextureFormat::D32_Float;
    desc.width = m_shadowMapSize;
    desc.height = m_shadowMapSize;
    desc.depth = 1;
    desc.mipLevels = 1;
    desc.arraySize = cascadeCount;
    desc.allowRenderTarget = false;   // 深度模板不使用 RTV
    desc.allowDepthStencil = true;
    desc.allowShaderResource = true;
    desc.allowUnorderedAccess = false;
    desc.name = "ShadowMap_" + std::to_string(lightIndex);

    auto texture = m_factory->CreateTextureImpl(desc);
    if (!texture) {
        return false;
    }

    m_shadowMapArrays[lightIndex] = std::move(texture);
    return true;
}

void ShadowMapManager::DestroyShadowTextures(uint32_t lightIndex) {
    if (lightIndex < m_shadowMapArrays.size()) {
        m_shadowMapArrays[lightIndex].reset();
    }
}

void ShadowMapManager::DestroyAllTextures() {
    m_shadowMapArrays.clear();
}

void ShadowMapManager::ComputeCascadeSplits(const PrismaMath::mat4& cameraProj) {
    float nearPlane = m_cascadeConfig.nearPlane;
    float farPlane = m_cascadeConfig.farPlane;
    uint32_t cascadeCount = m_cascadeConfig.cascadeCount;
    float lambda = m_cascadeConfig.splitLambda;

    m_cascadeSplits.resize(cascadeCount + 1);
    ComputePracticalSplits(cascadeCount, nearPlane, farPlane, lambda, m_cascadeSplits.data());
}

void ShadowMapManager::ComputeCascadeMatrices(uint32_t lightIndex,
                                               const PrismaMath::mat4& cameraView) {
    const auto& light = m_shadowLights[lightIndex];
    if (!light.castShadows) return;

    uint32_t cascadeCount = m_cascadeConfig.cascadeCount;
    uint32_t baseIdx = lightIndex * cascadeCount;

    // 从相机投影矩阵提取 FOV 和宽高比
    // 透视投影: proj[0][0] = 1/(aspect*tan(fov/2))
    float fovY = 2.0f * std::atan(1.0f / (cameraView[0][0] * 0.0f + 1.0f)); // fallback
    float aspectRatio = 1.0f;

    // 简化计算: 从视锥体角点提取
    std::array<PrismaMath::vec3, 8> worldCorners;

    for (uint32_t cascadeIdx = 0; cascadeIdx < cascadeCount; ++cascadeIdx) {
        float splitNear = m_cascadeSplits[cascadeIdx];
        float splitFar = m_cascadeSplits[cascadeIdx + 1];

        // 构建视空间视锥体角点
        PrismaMath::vec3 viewCorners[8];
        // 使用默认 FOV 45度，aspect 1.0 的近似构建
        // 实际应当从相机的投影矩阵精确计算
        {
            // 从投影矩阵精确解析
            float a = cameraProj[0][0]; // 1/(aspect * tan(fov/2))
            float b = cameraProj[1][1]; // 1/tan(fov/2)
            aspectRatio = b / a;
            float tanHalfFov = 1.0f / b;
            fovY = 2.0f * std::atan(tanHalfFov);
        }

        BuildFrustumCorners(splitNear, splitFar, fovY, aspectRatio, viewCorners);

        // 转换到世界空间
        PrismaMath::mat4 viewInverse = PrismaMath::inverse(cameraView);
        for (int i = 0; i < 8; ++i) {
            PrismaMath::vec4 worldPos = viewInverse * PrismaMath::vec4(viewCorners[i], 1.0f);
            worldCorners[i] = PrismaMath::vec3(worldPos) / worldPos.w;
        }

        // 计算光照空间 AABB
        PrismaMath::vec3 center, extents;
        ComputeLightSpaceAABB(worldCorners.data(), light.lightView, center, extents);

        // 创建级联的正交投影
        // 扩展 AABB 以稳定阴影边缘
        float texelsPerUnit = static_cast<float>(m_shadowMapSize) / (extents.x * 2.0f);
        center.x = std::round(center.x * texelsPerUnit) / texelsPerUnit;
        center.y = std::round(center.y * texelsPerUnit) / texelsPerUnit;

        float left   = center.x - extents.x;
        float right  = center.x + extents.x;
        float bottom = center.y - extents.y;
        float top    = center.y + extents.y;
        float nearZ  = center.z - extents.z;
        float farZ   = center.z + extents.z;

        // 扩展 Z 范围以包含所有阴影投射物
        nearZ -= 100.0f;
        farZ  += 100.0f;

        PrismaMath::mat4 lightProj = MakeOrthoMatrix(left, right, bottom, top, nearZ, farZ);

        // 存储级联军影矩阵: lightVP = lightProj * lightView
        uint32_t matrixIdx = baseIdx + cascadeIdx;
        if (matrixIdx < m_shadowMatrices.size()) {
            m_shadowMatrices[matrixIdx] = lightProj * light.lightView;
        }
    }
}

void ShadowMapManager::BuildFrustumCorners(uint32_t cascadeIndex,
                                            const PrismaMath::mat4& cameraView,
                                            PrismaMath::vec3 corners[8]) const {
    // 使用级联的分割距离构建
    float nearZ = m_cascadeSplits[cascadeIndex];
    float farZ = m_cascadeSplits[cascadeIndex + 1];

    // 从空间 view 矩阵计算基本参数
    float fovY = 45.0f * (3.14159265f / 180.0f); // 默认 45度
    float aspect = 1.0f;

    // 尝试从 view 位置附近提取 FOV (简化版)
    // 实际上在 Update() 中已计算，此处使用保守默认值

    BuildFrustumCorners(nearZ, farZ, fovY, aspect, corners);
}

} // namespace Prisma::Graphic
