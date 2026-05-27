#pragma once

#include "math/MathTypes.h"
#include "TerrainConfig.h"
#include <vector>
#include <string>
#include <memory>

namespace Prisma {
namespace Graphic { class TextureAtlas; }
namespace Terrain {

/**
 * @brief 地形纹理层 - 定义单个纹理层的混合参数
 */
struct TerrainTextureLayer {
    std::string name;           // 纹理名称（在 TextureAtlas 中）
    std::string texturePath;    // 纹理文件路径
    float blendHeight = 0.0f;   // 混合中心高度
    float blendWidth = 20.0f;   // 混合过渡宽度（越大过渡越平滑）
    float slopeFactor = 0.0f;   // 坡度影响 (0=不受坡度影响, 1=完全由坡度决定)
    Vector2 tiling = Vector2(32.0f); // UV 平铺

    TerrainTextureLayer() = default;

    TerrainTextureLayer(const std::string& name, const std::string& path,
                        float height, float width, float slope,
                        const Vector2& tiling)
        : name(name), texturePath(path), blendHeight(height)
        , blendWidth(width), slopeFactor(slope), tiling(tiling) {}
};

/**
 * @brief 地形材质 - 基于高度和坡度的纹理混合（纹理喷溅）
 *
 * 使用 4 层纹理（草、岩石、沙子、雪），根据高度和坡度混合。
 * 高度映射：低→沙/草，中→岩石，高→雪
 * 坡度映射：平缓→草，陡峭→岩石
 *
 * 输出 4 通道混合权重，可用于着色器纹理采样。
 */
class TerrainMaterial {
public:
    TerrainMaterial() = default;
    ~TerrainMaterial() = default;

    // ========== 层管理 ==========

    /**
     * @brief 添加纹理层
     */
    void AddLayer(const TerrainTextureLayer& layer);

    /**
     * @brief 设置指定索引的纹理层
     */
    void SetLayer(uint32_t index, const TerrainTextureLayer& layer);

    /**
     * @brief 获取指定索引的纹理层
     */
    const TerrainTextureLayer& GetLayer(uint32_t index) const;

    /**
     * @brief 获取层数量
     */
    size_t GetLayerCount() const { return m_Layers.size(); }

    /**
     * @brief 清空所有层
     */
    void ClearLayers();

    // ========== 混合权重计算 ==========

    /**
     * @brief 计算给定高度和坡度的混合权重
     * @param height 世界高度
     * @param slope 坡度（弧度）
     * @return 4 通道权重向量 (r=layer0, g=layer1, b=layer2, a=layer3)
     *
     * 每个通道对应一个纹理层的混合权重。
     * 所有权重之和约为 1.0。
     */
    Vector4 CalculateBlendWeights(float height, float slope) const;

    /**
     * @brief 计算顶点的 UV 坐标
     * @param worldX 世界 X
     * @param worldZ 世界 Z
     * @param layerIndex 纹理层索引
     * @return UV 坐标
     */
    Vector2 CalculateUV(float worldX, float worldZ, uint32_t layerIndex) const;

    // ========== 全局参数 ==========

    void SetMinHeight(float minHeight) { m_MinHeight = minHeight; }
    void SetMaxHeight(float maxHeight) { m_MaxHeight = maxHeight; }
    float GetMinHeight() const { return m_MinHeight; }
    float GetMaxHeight() const { return m_MaxHeight; }

    /**
     * @brief 从 TerrainConfig 初始化材质
     */
    void LoadFromConfig(const TerrainConfig& config);

    // ========== 纹理图集 ==========

    /**
     * @brief 设置纹理图集引用（用于获取纹理 UV 区域）
     */
    void SetTextureAtlas(Graphic::TextureAtlas* atlas) { m_Atlas = atlas; }

    /**
     * @brief 获取纹理图集指针
     */
    Graphic::TextureAtlas* GetTextureAtlas() const { return m_Atlas; }

private:
    std::vector<TerrainTextureLayer> m_Layers;
    float m_MinHeight = 0.0f;
    float m_MaxHeight = 255.0f;
    Graphic::TextureAtlas* m_Atlas = nullptr;

    /**
     * @brief 计算给定高度对应的高度混合权重
     */
    float HeightBlendWeight(float height, const TerrainTextureLayer& layer) const;

    /**
     * @brief 计算给定坡度对应的坡度混合权重
     */
    float SlopeBlendWeight(float slope, const TerrainTextureLayer& layer) const;
};

} // namespace Prisma::Terrain
} // namespace Prisma
