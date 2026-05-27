#pragma once

#include "math/MathTypes.h"
#include "TerrainConfig.h"
#include <vector>
#include <string>
#include <cstdint>
#include <limits>

namespace Prisma::Terrain {

/**
 * @brief 高度图 - 加载、采样和管理地形高度数据
 *
 * 支持 16-bit RAW、PNG 和原始 float 数组格式。
 * 使用双线性插值进行连续坐标采样。
 * 存储为 float 数组，范围为 [minHeight, maxHeight]。
 */
class HeightMap {
public:
    HeightMap() = default;
    ~HeightMap() = default;

    // ========== 加载方法 ==========

    /**
     * @brief 从 16-bit RAW 文件加载高度图
     * @param filepath RAW 文件路径
     * @param width 宽度（像素）
     * @param height 高度（像素）
     * @param heightScale 高度缩放因子
     * @return 是否成功
     */
    bool LoadFromRaw(const std::string& filepath, uint32_t width, uint32_t height,
                     float heightScale = 1.0f);

    /**
     * @brief 从 PNG 文件加载高度图
     * @param filepath PNG 文件路径
     * @param heightScale 高度缩放因子
     * @return 是否成功
     */
    bool LoadFromPNG(const std::string& filepath, float heightScale = 1.0f);

    /**
     * @brief 从原始 float 数组加载高度图
     * @param data 高度数据（行优先）
     * @param width 宽度
     * @param height 高度
     * @return 是否成功
     */
    bool LoadFromFloatArray(const float* data, uint32_t width, uint32_t height);

    /**
     * @brief 从 float vector 加载高度图
     * @param data 高度数据（行优先）
     * @param width 宽度
     * @param height 高度
     * @return 是否成功
     */
    bool LoadFromFloatVector(const std::vector<float>& data, uint32_t width, uint32_t height);

    /**
     * @brief 生成测试用简单高度图（正弦波山丘）
     * @param width 宽度
     * @param height 高度
     * @param frequency 频率
     * @param amplitude 振幅
     */
    void GenerateTestTerrain(uint32_t width, uint32_t height,
                             float frequency = 0.02f, float amplitude = 40.0f);

    /**
     * @brief 生成平缓平原
     * @param width 宽度
     * @param height 高度
     * @param baseHeight 基础高度
     */
    void GenerateFlat(uint32_t width, uint32_t height, float baseHeight = 0.0f);

    // ========== 采样方法 ==========

    /**
     * @brief 获取指定世界坐标的高度
     * @param worldX 世界 X 坐标
     * @param worldZ 世界 Z 坐标
     * @return 插值后的高度值
     *
     * 将世界坐标映射到高度图 UV 空间，执行双线性插值。
     * 世界坐标原点 (0,0) 对应高度图中心。
     */
    float GetHeightAt(float worldX, float worldZ) const;

    /**
     * @brief 获取指定归一化坐标的高度
     * @param u 归一化 X [0, 1]
     * @param v 归一化 Z [0, 1]
     * @return 插值后的高度值
     */
    float Sample(float u, float v) const;

    /**
     * @brief 获取指定像素坐标的高度（离散采样，无插值）
     * @param x 像素 X
     * @param y 像素 Y
     * @return 高度值
     */
    float GetHeightAtPixel(uint32_t x, uint32_t y) const;

    /**
     * @brief 获取世界坐标处的法线方向
     * @param worldX 世界 X 坐标
     * @param worldZ 世界 Z 坐标
     * @return 归一化法线向量
     *
     * 使用中心差分法计算坡度，然后推导法线。
     */
    Vector3 GetNormalAt(float worldX, float worldZ) const;

    /**
     * @brief 获取像素坐标处的法线（使用相邻像素）
     * @param x 像素 X
     * @param y 像素 Y
     * @return 归一化法线向量
     */
    Vector3 GetNormalAtPixel(uint32_t x, uint32_t y) const;

    // ========== 属性查询 ==========

    float GetMinHeight() const { return m_MinHeight; }
    float GetMaxHeight() const { return m_MaxHeight; }
    float GetHeightRange() const { return m_MaxHeight - m_MinHeight; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    const float* GetData() const { return m_HeightData.data(); }
    size_t GetDataSize() const { return m_HeightData.size(); }
    bool IsValid() const { return !m_HeightData.empty(); }

    /**
     * @brief 将世界坐标映射到高度图 UV
     * @param worldX 世界 X
     * @param worldZ 世界 Z
     * @return UV 坐标 (u, v)
     */
    Vector2 WorldToUV(float worldX, float worldZ) const;

    /**
     * @brief 将 UV 映射到世界坐标
     * @param u [0, 1]
     * @param v [0, 1]
     * @return 世界坐标 (x, z)
     */
    Vector2 UVToWorld(float u, float v) const;

    /**
     * @brief 导出碰撞网格顶点
     * @param resolution 采样步长（世界单位），默认 1.0
     * @param yOffset Y 偏移
     * @return 顶点数组
     */
    std::vector<Vector3> ExportCollisionMesh(float resolution = 1.0f,
                                              float yOffset = 0.0f) const;

    /**
     * @brief 获取整个高度图的 AABB
     * @param yOffset 额外 Y 偏移
     * @return (min, max) 包围盒
     */
    std::pair<Vector3, Vector3> GetBounds(float yOffset = 0.0f) const;

private:
    /**
     * @brief 双线性插值采样
     * @param x 像素 X 坐标（浮点，支持小数）
     * @param y 像素 Y 坐标（浮点，支持小数）
     * @return 插值高度
     */
    float BilinearSample(float x, float y) const;

    /**
     * @brief 更新最小/最大高度统计
     */
    void UpdateHeightRange();

    std::vector<float> m_HeightData;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    float m_MinHeight = std::numeric_limits<float>::max();
    float m_MaxHeight = std::numeric_limits<float>::lowest();

    // 世界空间映射参数
    float m_WorldScale = 1.0f; // 1 世界单位 = 1 高度图单位
};

} // namespace Prisma::Terrain
