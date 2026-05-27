#pragma once

#include "math/MathTypes.h"
#include "core/Timestep.h"
#include "Export.h"
#include <vector>
#include <random>
#include <functional>

namespace Prisma::Particles {

// 粒子结构
struct ENGINE_API Particle {
    Vector3 position      = Vector3(0.0f);
    Vector3 velocity      = Vector3(0.0f);
    Vector4 color         = Vector4(1.0f); // rgba
    float   size          = 1.0f;
    float   lifetime      = 1.0f; // 总生命周期（秒）
    float   age           = 0.0f; // 当前年龄（秒）
    float   rotation      = 0.0f; // 旋转角度（弧度）
    float   angularVelocity = 0.0f;

    bool IsAlive() const { return age < lifetime; }
    float GetNormalizedAge() const { return lifetime > 0.0f ? glm::clamp(age / lifetime, 0.0f, 1.0f) : 1.0f; }
};

// 发射器形状枚举
enum class EmitterShape {
    Point,   // 点
    Sphere,  // 球壳/球体
    Box,     // 盒体
    Cone     // 锥体
};

// 混合模式枚举
enum class ParticleBlendMode {
    Alpha,    // 常规透明混合
    Additive  // 叠加混合
};

// 随机范围结构
template<typename T>
struct Range {
    T min;
    T max;

    Range() : min(T(0)), max(T(0)) {}
    Range(const T& v) : min(v), max(v) {}
    Range(const T& mn, const T& mx) : min(mn), max(mx) {}

    T Random(std::mt19937& rng) const {
        if constexpr (std::is_same_v<T, float>) {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(rng);
        } else if constexpr (std::is_same_v<T, Vector3>) {
            return Vector3(
                Range<float>(min.x, max.x).Random(rng),
                Range<float>(min.y, max.y).Random(rng),
                Range<float>(min.z, max.z).Random(rng)
            );
        } else if constexpr (std::is_same_v<T, Vector4>) {
            return Vector4(
                Range<float>(min.x, max.x).Random(rng),
                Range<float>(min.y, max.y).Random(rng),
                Range<float>(min.z, max.z).Random(rng),
                Range<float>(min.w, max.w).Random(rng)
            );
        }
        return T(0);
    }

    T Mid() const { return (min + max) * T(0.5); }
};

// 粒子生命周期曲线
struct ParticleCurve {
    // 颜色渐变关键帧 (age [0,1] -> color)
    std::vector<std::pair<float, Vector4>> colorKeys;
    // 尺寸缩放关键帧 (age [0,1] -> scale)
    std::vector<std::pair<float, float>>   sizeKeys;

    Vector4 EvaluateColor(float t) const;
    float   EvaluateSize(float t) const;
};

// 发射器配置
struct ENGINE_API EmitterConfig {
    // 基础发射参数
    float     spawnRate       = 10.0f;   // 每秒钟发射粒子数
    uint32_t  burstCount      = 0;       // 初始爆发数量（0 = 无爆发）
    EmitterShape shape        = EmitterShape::Point;

    // 空间参数
    Vector3 position          = Vector3(0.0f);
    Vector3 forward           = Vector3(0.0f, 1.0f, 0.0f); // 锥体方向
    float   radius            = 1.0f;   // 球体/锥体底部半径
    Vector3 boxExtents        = Vector3(1.0f); // 盒体半范围
    float   coneAngle         = 45.0f;  // 锥体半角（度）

    // 生命周期
    Range<float> lifetime     = Range<float>(1.0f, 3.0f);

    // 速度
    Range<float> speed        = Range<float>(1.0f, 5.0f);
    Vector3      gravity      = Vector3(0.0f, -9.81f, 0.0f);

    // 外观变化
    Range<Vector4> startColor = Range<Vector4>(Vector4(1.0f));
    Range<Vector4> endColor   = Range<Vector4>(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
    Range<float>   startSize  = Range<float>(0.5f, 1.5f);
    Range<float>   endSize    = Range<float>(0.0f, 0.1f);

    // 旋转
    Range<float> rotation         = Range<float>(0.0f, 360.0f);
    Range<float> angularVelocity  = Range<float>(-180.0f, 180.0f); // 度/秒

    // 混合模式
    ParticleBlendMode blendMode = ParticleBlendMode::Alpha;

    // 生命周期曲线（可覆盖简单的 start/end 颜色和尺寸）
    ParticleCurve curve;

    // LOD
    float maxDistance = 100.0f; // 超过此距离停止发射
    float lodDistance = 50.0f;  // 超过此距离减少粒子数
    float lodFactor   = 0.5f;   // LOD 距离下的发射比例
};

// CPU 粒子系统
class ENGINE_API CPUParticleSystem {
public:
    CPUParticleSystem();
    ~CPUParticleSystem();

    // 核心更新（每帧调用）
    void Update(float dt, const Vector3& cameraPosition);

    // 发射粒子
    void Spawn(uint32_t count, const EmitterConfig& config, std::mt19937& rng);
    void SpawnBurst(const EmitterConfig& config, std::mt19937& rng);

    // 重置 / 清空
    void Clear();
    void Reset();

    // 访问粒子
    const std::vector<Particle>& GetParticles() const { return m_Particles; }
    std::vector<Particle>& GetParticles() { return m_Particles; }

    // 粒子计数
    size_t GetAliveCount() const;
    size_t GetTotalCount() const { return m_Particles.size(); }
    size_t GetCapacity() const { return m_Capacity; }
    void   SetCapacity(size_t capacity) { m_Capacity = capacity; m_Particles.reserve(capacity); }

    // 距离排序（绘制前调用，确保透明混合正确）
    void SortByDistance(const Vector3& cameraPosition);
    bool IsSortEnabled() const { return m_SortEnabled; }
    void SetSortEnabled(bool enabled) { m_SortEnabled = enabled; }

    // LOD
    float GetLodFactor() const { return m_LodFactor; }
    void  SetLodFactor(float factor) { m_LodFactor = glm::clamp(factor, 0.0f, 1.0f); }

private:
    std::vector<Particle> m_Particles;
    size_t m_Capacity = 10000;
    bool   m_SortEnabled = true;
    float  m_LodFactor = 1.0f;
};

// EmitterConfig 辅助：在形状内生成随机位置
Vector3 RandomPositionInShape(const EmitterConfig& config, std::mt19937& rng);
// EmitterConfig 辅助：在形状内生成随机方向
Vector3 RandomDirectionInShape(const EmitterConfig& config, std::mt19937& rng);

} // namespace Prisma::Particles
