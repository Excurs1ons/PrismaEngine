#include "terrain/TerrainMaterial.h"
#include "graphic/TextureAtlas.h"
#include <algorithm>
#include <cmath>

namespace Prisma::Terrain {

// ============================================================================
// 层管理
// ============================================================================

void TerrainMaterial::AddLayer(const TerrainTextureLayer& layer)
{
    m_Layers.push_back(layer);
}

void TerrainMaterial::SetLayer(uint32_t index, const TerrainTextureLayer& layer)
{
    if (index < m_Layers.size()) {
        m_Layers[index] = layer;
    }
}

const TerrainTextureLayer& TerrainMaterial::GetLayer(uint32_t index) const
{
    static TerrainTextureLayer s_Default;
    if (index < m_Layers.size()) {
        return m_Layers[index];
    }
    return s_Default;
}

void TerrainMaterial::ClearLayers()
{
    m_Layers.clear();
}

// ============================================================================
// 混合权重计算
// ============================================================================

Vector4 TerrainMaterial::CalculateBlendWeights(float height, float slope) const
{
    Vector4 weights(0.0f);

    if (m_Layers.empty()) {
        weights.x = 1.0f; // 默认只使用第一个层
        return weights;
    }

    // 累计所有层的混合权重
    for (size_t i = 0; i < m_Layers.size() && i < 4; ++i) {
        float hw = HeightBlendWeight(height, m_Layers[i]);
        float sw = SlopeBlendWeight(slope, m_Layers[i]);

        // 组合高度和坡度权重：坡度影响越大，高度影响越小
        float combined = glm::mix(hw, sw, m_Layers[i].slopeFactor);
        weights[static_cast<int>(i)] = combined;
    }

    // 归一化权重
    float total = weights.x + weights.y + weights.z + weights.w;
    if (total > 0.0f) {
        weights /= total;
    } else {
        // 如果所有权重都为 0，使用第一个层
        weights.x = 1.0f;
    }

    return weights;
}

Vector2 TerrainMaterial::CalculateUV(float worldX, float worldZ,
                                      uint32_t layerIndex) const
{
    if (layerIndex >= m_Layers.size()) {
        return Vector2(worldX, worldZ);
    }

    const auto& layer = m_Layers[layerIndex];
    return Vector2(
        worldX / layer.tiling.x,
        worldZ / layer.tiling.y
    );
}

void TerrainMaterial::LoadFromConfig(const TerrainConfig& config)
{
    m_Layers.clear();

    for (const auto& layerDef : config.textureLayers) {
        TerrainTextureLayer layer;
        layer.name = layerDef.name;
        layer.texturePath = layerDef.texturePath;
        layer.blendHeight = layerDef.blendHeight;
        layer.blendWidth = layerDef.blendWidth;
        layer.slopeFactor = layerDef.slopeFactor;
        layer.tiling = layerDef.tiling;
        m_Layers.push_back(layer);
    }

    if (m_Layers.empty()) {
        // 填充默认 4 层
        m_Layers = {
            { "grass", "textures/terrain/grass.png", 10.0f,  15.0f, 0.0f, Vector2(32.0f) },
            { "rock",  "textures/terrain/rock.png",  50.0f,  20.0f, 0.7f, Vector2(32.0f) },
            { "sand",  "textures/terrain/sand.png",  5.0f,   10.0f, 0.0f, Vector2(64.0f) },
            { "snow",  "textures/terrain/snow.png",  100.0f, 15.0f, 0.3f, Vector2(64.0f) }
        };
    }
}

// ============================================================================
// 内部方法
// ============================================================================

float TerrainMaterial::HeightBlendWeight(float height,
                                          const TerrainTextureLayer& layer) const
{
    float range = m_MaxHeight - m_MinHeight;
    if (range <= 0.0f) return (height >= layer.blendHeight) ? 1.0f : 0.0f;

    // 使用平滑的 S 曲线过渡
    float normalizedH = (height - layer.blendHeight) / layer.blendWidth;
    float weight = 0.5f + 0.5f * std::tanh(normalizedH * 2.0f);

    return std::clamp(weight, 0.0f, 1.0f);
}

float TerrainMaterial::SlopeBlendWeight(float slope,
                                         const TerrainTextureLayer& layer) const
{
    // 坡度层：陡峭区域（slope 大）权重高
    // 非坡度层：平坦区域（slope 小）权重高
    float normalizedSlope = slope / (glm::half_pi<float>() * 0.5f);

    if (layer.slopeFactor > 0.5f) {
        // 坡度敏感层（如岩石）：陡峭区域权重高
        return std::clamp(normalizedSlope, 0.0f, 1.0f);
    } else {
        // 非坡度敏感层（如草地）：平坦区域权重高
        return std::clamp(1.0f - normalizedSlope, 0.0f, 1.0f);
    }
}

} // namespace Prisma::Terrain
