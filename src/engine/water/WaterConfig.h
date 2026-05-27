#pragma once

#include "math/MathTypes.h"
#include <cstdint>
#include <vector>

namespace Prisma::Water {

/**
 * @brief Gerstner wave component - 单个格斯特纳波参数
 */
struct GerstnerWave {
    Vector2 direction = Vector2(1.0f, 0.0f);  // 波传播方向（将被归一化）
    float amplitude   = 1.0f;                  // 波幅（米）
    float frequency   = 0.5f;                  // 角频率 (rad/s)
    float speed       = 2.0f;                  // 传播速度 (m/s)
    float steepness   = 0.3f;                  // 陡度 [0, 1)，越大波峰越尖

    GerstnerWave() = default;

    GerstnerWave(const Vector2& dir, float amp, float freq, float spd, float steep)
        : direction(dir), amplitude(amp), frequency(freq), speed(spd), steepness(steep) {}
};

/**
 * @brief FFT wave simulation parameters
 */
struct FFTWaveConfig {
    uint32_t resolution = 256;          // 位移图分辨率 (建议 128/256/512)
    float patchSize     = 100.0f;       // 模拟区域世界大小 (m)
    float windSpeed     = 10.0f;        // 风速 (m/s)
    Vector2 windDirection = Vector2(1.0f, 0.0f); // 风向（归一化）
    float amplitude     = 0.5f;         // 波幅倍率
    float lambda        = 1.0f;         // 风涌因子 (Choppy waves)
    float loopDuration  = 200.0f;       // 循环周期 (s)，越大越不重复
};

/**
 * @brief 水渲染配置
 */
struct WaterRenderConfig {
    // ========== 水面参数 ==========
    float waterLevel       = 0.0f;      // 水面高度 (Y)
    float waveHeightScale  = 1.0f;      // 波高整体缩放
    Vector2 tileSize       = Vector2(200.0f, 200.0f); // 水面网格世界大小
    uint32_t baseSegments  = 128;       // 基础网格细分段数
    uint32_t farSegments   = 16;        // 远处细分段数
    float lodDistance      = 100.0f;    // LOD 过渡距离

    // ========== 外观参数 ==========
    Vector3 shallowColor   = Vector3(0.2f, 0.5f, 0.6f);  // 浅水色
    Vector3 deepColor      = Vector3(0.0f, 0.05f, 0.2f); // 深水色
    float shallowDepth     = 2.0f;      // 浅水深(m)
    float deepDepth        = 20.0f;     // 深水深(m)，用于颜色混合

    float fresnelPower     = 5.0f;      // 菲涅尔指数
    float fresnelStrength  = 0.5f;      // 菲涅尔强度
    float specularStrength = 0.8f;      // 镜面反射强度
    float specularPower    = 64.0f;     // 镜面反射光泽度

    float transparency     = 0.6f;      // 透明度 [0,1]
    float refractionScale  = 0.02f;     // 折射扭曲幅度

    // ========== 波光粼粼 ==========
    float sunGlowStrength  = 0.3f;      // 波光强度
    float sunGlowPower     = 32.0f;     // 波光集中度

    // ========== 深度雾 ==========
    float fogDensity       = 0.015f;    // 雾密度
    Vector3 fogColor       = Vector3(0.1f, 0.2f, 0.3f); // 雾色
};

/**
 * @brief 水体模拟完整配置
 */
struct WaterConfig {
    // 渲染配置
    WaterRenderConfig render;

    // Gerstner 波列
    std::vector<GerstnerWave> gerstnerWaves;

    // FFT 配置（GPU 模式使用）
    bool useFFT = false;
    FFTWaveConfig fft;

    // 交互配置
    bool enableInteraction = true;
    float rippleDecay      = 2.0f;      // 涟漪衰减速度
    uint32_t maxRipples    = 64;         // 最大涟漪数
    float rippleSpeed      = 5.0f;      // 涟漪扩散速度 (m/s)

    // ========== 默认构造函数 ==========
    WaterConfig() {
        // 填充默认 Gerstner 波列（8 波）
        gerstnerWaves = {
            { Vector2( 1.0f,  0.5f), 1.2f, 0.8f,  2.0f, 0.3f },
            { Vector2( 0.5f,  1.0f), 0.8f, 1.2f,  1.5f, 0.2f },
            { Vector2(-0.3f,  1.0f), 0.6f, 1.5f,  3.0f, 0.4f },
            { Vector2( 1.0f, -0.3f), 0.5f, 0.6f,  2.5f, 0.2f },
            { Vector2(-0.5f, -0.8f), 0.4f, 1.0f,  1.8f, 0.3f },
            { Vector2( 0.8f, -0.5f), 0.3f, 1.8f,  3.5f, 0.1f },
            { Vector2(-0.7f,  0.7f), 0.3f, 2.2f,  4.0f, 0.2f },
            { Vector2( 0.2f, -0.9f), 0.2f, 0.9f,  1.2f, 0.3f },
        };
    }
};

} // namespace Prisma::Water
