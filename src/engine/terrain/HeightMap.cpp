#include "terrain/HeightMap.h"
#include "Logger.h"
#include <stb_image.h>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Prisma::Terrain {

// ============================================================================
// 加载方法
// ============================================================================

bool HeightMap::LoadFromRaw(const std::string& filepath, uint32_t width,
                             uint32_t height, float heightScale)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        LOG_ERROR("Terrain", "无法打开 RAW 高度图文件: {}", filepath);
        return false;
    }

    // 16-bit RAW: width * height * sizeof(uint16_t) 字节
    size_t expectedSize = static_cast<size_t>(width) * height * sizeof(uint16_t);
    file.seekg(0, std::ios::end);
    size_t actualSize = static_cast<size_t>(file.tellg());

    if (actualSize < expectedSize) {
        LOG_ERROR("Terrain", "RAW 文件大小不足: 期望 {} 字节, 实际 {} 字节",
                  expectedSize, actualSize);
        return false;
    }
    file.seekg(0, std::ios::beg);

    // 读取 uint16 数据
    std::vector<uint16_t> rawData(width * height);
    file.read(reinterpret_cast<char*>(rawData.data()),
              static_cast<std::streamsize>(width * height * sizeof(uint16_t)));
    file.close();

    // 转换为 float 并缩放
    m_Width = width;
    m_Height = height;
    m_HeightData.resize(static_cast<size_t>(width) * height);

    for (size_t i = 0; i < rawData.size(); ++i) {
        m_HeightData[i] = static_cast<float>(rawData[i]) * heightScale;
    }

    UpdateHeightRange();
    LOG_INFO("Terrain", "已加载 RAW 高度图: {} ({}x{}), 高度范围 [{:.1f}, {:.1f}]",
             filepath, width, height, m_MinHeight, m_MaxHeight);
    return true;
}

bool HeightMap::LoadFromPNG(const std::string& filepath, float heightScale)
{
    int imgWidth, imgHeight, channels;
    unsigned char* imgData = stbi_load(filepath.c_str(), &imgWidth, &imgHeight,
                                        &channels, 1); // 强制单通道
    if (!imgData) {
        LOG_ERROR("Terrain", "无法加载 PNG 高度图: {} ({})",
                  filepath, stbi_failure_reason());
        return false;
    }

    m_Width = static_cast<uint32_t>(imgWidth);
    m_Height = static_cast<uint32_t>(imgHeight);
    m_HeightData.resize(static_cast<size_t>(m_Width) * m_Height);

    // 8-bit PNG: 将 [0, 255] 映射到 [0, 1] 再乘以高度缩放
    for (int i = 0; i < imgWidth * imgHeight; ++i) {
        m_HeightData[i] = (static_cast<float>(imgData[i]) / 255.0f) * heightScale;
    }

    stbi_image_free(imgData);

    UpdateHeightRange();
    LOG_INFO("Terrain", "已加载 PNG 高度图: {} ({}x{}), 高度范围 [{:.1f}, {:.1f}]",
             filepath, m_Width, m_Height, m_MinHeight, m_MaxHeight);
    return true;
}

bool HeightMap::LoadFromFloatArray(const float* data, uint32_t width, uint32_t height)
{
    if (!data || width == 0 || height == 0) {
        LOG_ERROR("Terrain", "无效的高度图数据 (data={}, w={}, h={})",
                  static_cast<const void*>(data), width, height);
        return false;
    }

    m_Width = width;
    m_Height = height;
    m_HeightData.assign(data, data + static_cast<size_t>(width) * height);

    UpdateHeightRange();
    LOG_INFO("Terrain", "已从 float 数组加载高度图: {}x{}, 范围 [{:.1f}, {:.1f}]",
             width, height, m_MinHeight, m_MaxHeight);
    return true;
}

bool HeightMap::LoadFromFloatVector(const std::vector<float>& data,
                                     uint32_t width, uint32_t height)
{
    if (data.size() != static_cast<size_t>(width) * height) {
        LOG_ERROR("Terrain", "Float vector 大小不匹配: 期望 {} 像素, 实际 {}",
                  width * height, data.size());
        return false;
    }
    return LoadFromFloatArray(data.data(), width, height);
}

void HeightMap::GenerateTestTerrain(uint32_t width, uint32_t height,
                                     float frequency, float amplitude)
{
    m_Width = width;
    m_Height = height;
    m_HeightData.resize(static_cast<size_t>(width) * height);

    for (uint32_t z = 0; z < height; ++z) {
        for (uint32_t x = 0; x < width; ++x) {
            float fx = static_cast<float>(x) - static_cast<float>(width) * 0.5f;
            float fz = static_cast<float>(z) - static_cast<float>(height) * 0.5f;

            // 多个正弦波叠加制造自然地形
            float h = 0.0f;
            h += std::sin(fx * frequency) * std::cos(fz * frequency) * amplitude * 0.6f;
            h += std::sin(fx * frequency * 2.3f + 1.5f) * std::cos(fz * frequency * 1.7f + 0.8f) * amplitude * 0.3f;
            h += std::sin(fx * frequency * 0.5f) * amplitude * 0.1f;

            m_HeightData[static_cast<size_t>(z) * width + x] = h + amplitude * 0.5f;
        }
    }

    UpdateHeightRange();
    LOG_INFO("Terrain", "已生成测试地形: {}x{}, 频率={:.3f}, 振幅={:.1f}, 范围 [{:.1f}, {:.1f}]",
             width, height, frequency, amplitude, m_MinHeight, m_MaxHeight);
}

void HeightMap::GenerateFlat(uint32_t width, uint32_t height, float baseHeight)
{
    m_Width = width;
    m_Height = height;
    m_HeightData.assign(static_cast<size_t>(width) * height, baseHeight);
    UpdateHeightRange();
    LOG_INFO("Terrain", "已生成平坦地形: {}x{}, 高度={:.1f}", width, height, baseHeight);
}

// ============================================================================
// 采样方法
// ============================================================================

float HeightMap::GetHeightAt(float worldX, float worldZ) const
{
    if (!IsValid()) return 0.0f;

    // 将世界坐标映射到高度图像素空间
    Vector2 uv = WorldToUV(worldX, worldZ);

    // 将 [0,1] UV 映射到 [0, width-1] / [0, height-1] 像素空间
    float pixelX = uv.x * static_cast<float>(m_Width - 1);
    float pixelY = uv.y * static_cast<float>(m_Height - 1);

    return BilinearSample(pixelX, pixelY);
}

float HeightMap::Sample(float u, float v) const
{
    if (!IsValid()) return 0.0f;

    // 将 [0,1] UV 映射到像素空间
    float pixelX = u * static_cast<float>(m_Width - 1);
    float pixelY = v * static_cast<float>(m_Height - 1);

    return BilinearSample(pixelX, pixelY);
}

float HeightMap::GetHeightAtPixel(uint32_t x, uint32_t y) const
{
    if (!IsValid()) return 0.0f;
    x = std::min(x, m_Width - 1);
    y = std::min(y, m_Height - 1);
    return m_HeightData[static_cast<size_t>(y) * m_Width + x];
}

Vector2 HeightMap::WorldToUV(float worldX, float worldZ) const
{
    if (!IsValid()) return Vector2(0.5f);

    // 世界坐标 (0,0) 对应高度图中心
    float halfWidth = static_cast<float>(m_Width) * 0.5f * m_WorldScale;
    float halfHeight = static_cast<float>(m_Height) * 0.5f * m_WorldScale;

    float u = (worldX / m_WorldScale + halfWidth) / static_cast<float>(m_Width);
    float v = (worldZ / m_WorldScale + halfHeight) / static_cast<float>(m_Height);

    // 钳制到 [0, 1]
    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);

    return Vector2(u, v);
}

Vector2 HeightMap::UVToWorld(float u, float v) const
{
    float halfWidth = static_cast<float>(m_Width) * 0.5f * m_WorldScale;
    float halfHeight = static_cast<float>(m_Height) * 0.5f * m_WorldScale;

    float worldX = (u * static_cast<float>(m_Width) - halfWidth) * m_WorldScale;
    float worldZ = (v * static_cast<float>(m_Height) - halfHeight) * m_WorldScale;

    return Vector2(worldX, worldZ);
}

// ============================================================================
// 法线计算
// ============================================================================

Vector3 HeightMap::GetNormalAt(float worldX, float worldZ) const
{
    if (!IsValid()) return Vector3(0.0f, 1.0f, 0.0f);

    // 使用中心差分法计算坡度
    const float delta = 0.5f; // 采样步长（世界单位）

    float hL = GetHeightAt(worldX - delta, worldZ);
    float hR = GetHeightAt(worldX + delta, worldZ);
    float hD = GetHeightAt(worldX, worldZ - delta);
    float hU = GetHeightAt(worldX, worldZ + delta);

    // 坡度向量（地形梯度的负方向）
    Vector3 slope(
        (hL - hR) / (2.0f * delta),
        2.0f, // Y 分量 = 2（标准化前的向上方向强度）
        (hD - hU) / (2.0f * delta)
    );

    return glm::normalize(slope);
}

Vector3 HeightMap::GetNormalAtPixel(uint32_t x, uint32_t y) const
{
    if (!IsValid()) return Vector3(0.0f, 1.0f, 0.0f);

    x = std::min(x, m_Width - 1);
    y = std::min(y, m_Height - 1);

    // 边界处理：如果 x/y 在边界，使用偏移方向
    uint32_t xL = (x == 0) ? x : x - 1;
    uint32_t xR = (x >= m_Width - 1) ? x : x + 1;
    uint32_t yD = (y == 0) ? y : y - 1;
    uint32_t yU = (y >= m_Height - 1) ? y : y + 1;

    float hL = m_HeightData[static_cast<size_t>(y) * m_Width + xL];
    float hR = m_HeightData[static_cast<size_t>(y) * m_Width + xR];
    float hD = m_HeightData[static_cast<size_t>(yD) * m_Width + x];
    float hU = m_HeightData[static_cast<size_t>(yU) * m_Width + x];

    float scaleX = (xR == xL) ? 1.0f : static_cast<float>(xR - xL);
    float scaleZ = (yU == yD) ? 1.0f : static_cast<float>(yU - yD);

    Vector3 slope(
        (hL - hR) / scaleX,
        2.0f,
        (hD - hU) / scaleZ
    );

    return glm::normalize(slope);
}

// ============================================================================
// 导出和工具
// ============================================================================

std::vector<Vector3> HeightMap::ExportCollisionMesh(float resolution,
                                                     float yOffset) const
{
    std::vector<Vector3> vertices;
    if (!IsValid()) return vertices;

    // 生成规则网格顶点
    for (float z = 0; z < static_cast<float>(m_Height); z += resolution) {
        uint32_t iz = static_cast<uint32_t>(std::floor(z));
        for (float x = 0; x < static_cast<float>(m_Width); x += resolution) {
            uint32_t ix = static_cast<uint32_t>(std::floor(x));
            Vector2 world = UVToWorld(
                static_cast<float>(ix) / static_cast<float>(m_Width - 1),
                static_cast<float>(iz) / static_cast<float>(m_Height - 1)
            );
            float height = GetHeightAtPixel(ix, iz);
            vertices.emplace_back(world.x, height + yOffset, world.y);
        }
    }

    return vertices;
}

std::pair<Vector3, Vector3> HeightMap::GetBounds(float yOffset) const
{
    if (!IsValid()) {
        return {Vector3(-1.0f), Vector3(1.0f)};
    }

    float halfWidth = static_cast<float>(m_Width) * 0.5f * m_WorldScale;
    float halfHeight = static_cast<float>(m_Height) * 0.5f * m_WorldScale;

    Vector3 min(-halfWidth, m_MinHeight + yOffset, -halfHeight);
    Vector3 max(halfWidth, m_MaxHeight + yOffset, halfHeight);

    return {min, max};
}

// ============================================================================
// 内部方法
// ============================================================================

float HeightMap::BilinearSample(float x, float y) const
{
    // 钳制到有效范围
    x = std::clamp(x, 0.0f, static_cast<float>(m_Width - 1));
    y = std::clamp(y, 0.0f, static_cast<float>(m_Height - 1));

    uint32_t x0 = static_cast<uint32_t>(std::floor(x));
    uint32_t y0 = static_cast<uint32_t>(std::floor(y));
    uint32_t x1 = std::min(x0 + 1, m_Width - 1);
    uint32_t y1 = std::min(y0 + 1, m_Height - 1);

    float fracX = x - static_cast<float>(x0);
    float fracY = y - static_cast<float>(y0);

    // 四个角的高度
    float h00 = m_HeightData[static_cast<size_t>(y0) * m_Width + x0];
    float h10 = m_HeightData[static_cast<size_t>(y0) * m_Width + x1];
    float h01 = m_HeightData[static_cast<size_t>(y1) * m_Width + x0];
    float h11 = m_HeightData[static_cast<size_t>(y1) * m_Width + x1];

    // 双线性插值
    float top = glm::mix(h00, h10, fracX);
    float bottom = glm::mix(h01, h11, fracX);

    return glm::mix(top, bottom, fracY);
}

void HeightMap::UpdateHeightRange()
{
    if (m_HeightData.empty()) {
        m_MinHeight = 0.0f;
        m_MaxHeight = 0.0f;
        return;
    }

    auto [minIt, maxIt] = std::minmax_element(m_HeightData.begin(),
                                               m_HeightData.end());
    m_MinHeight = *minIt;
    m_MaxHeight = *maxIt;
}

} // namespace Prisma::Terrain
