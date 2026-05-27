#include "particles/ParticleSystem.h"
#include "Logger.h"
#include "Engine.h"
#include "transform/Transform.h"
#include <algorithm>

namespace Prisma::Particles {

// ============================================================================
// ParticleCurve 辅助函数
// ============================================================================

Vector4 ParticleCurve::EvaluateColor(float t) const {
    if (colorKeys.empty()) return Vector4(1.0f);
    if (colorKeys.size() == 1 || t <= colorKeys[0].first) return colorKeys[0].second;

    for (size_t i = 0; i < colorKeys.size() - 1; ++i) {
        if (t >= colorKeys[i].first && t < colorKeys[i + 1].first) {
            float segment = (t - colorKeys[i].first) / (colorKeys[i + 1].first - colorKeys[i].first);
            return glm::mix(colorKeys[i].second, colorKeys[i + 1].second, segment);
        }
    }
    return colorKeys.back().second;
}

float ParticleCurve::EvaluateSize(float t) const {
    if (sizeKeys.empty()) return 1.0f;
    if (sizeKeys.size() == 1 || t <= sizeKeys[0].first) return sizeKeys[0].second;

    for (size_t i = 0; i < sizeKeys.size() - 1; ++i) {
        if (t >= sizeKeys[i].first && t < sizeKeys[i + 1].first) {
            float segment = (t - sizeKeys[i].first) / (sizeKeys[i + 1].first - sizeKeys[i].first);
            return glm::mix(sizeKeys[i].second, sizeKeys[i + 1].second, segment);
        }
    }
    return sizeKeys.back().second;
}

// ============================================================================
// 发射形状辅助函数
// ============================================================================

Vector3 RandomPositionInShape(const EmitterConfig& config, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    switch (config.shape) {
    case EmitterShape::Point:
        return config.position;

    case EmitterShape::Sphere: {
        // 球体内均匀分布
        float theta = dist01(rng) * glm::two_pi<float>();
        float phi = std::acos(2.0f * dist01(rng) - 1.0f);
        float r = config.radius * std::cbrt(dist01(rng));
        return config.position + Vector3(
            r * std::sin(phi) * std::cos(theta),
            r * std::cos(phi),
            r * std::sin(phi) * std::sin(theta)
        );
    }

    case EmitterShape::Box: {
        return config.position + Vector3(
            config.boxExtents.x * (2.0f * dist01(rng) - 1.0f),
            config.boxExtents.y * (2.0f * dist01(rng) - 1.0f),
            config.boxExtents.z * (2.0f * dist01(rng) - 1.0f)
        );
    }

    case EmitterShape::Cone: {
        // 锥体底部圆盘范围内随机位置
        float angle = dist01(rng) * glm::two_pi<float>();
        float radius = config.radius * std::sqrt(dist01(rng));
        Vector3 right = glm::normalize(glm::cross(config.forward, Vector3(0.0f, 1.0f, 0.0f)));
        if (glm::length(right) < 0.001f) {
            right = glm::normalize(glm::cross(config.forward, Vector3(1.0f, 0.0f, 0.0f)));
        }
        Vector3 up = glm::normalize(glm::cross(right, config.forward));
        return config.position
            + right * (radius * std::cos(angle))
            + up * (radius * std::sin(angle));
    }

    default:
        return config.position;
    }
}

Vector3 RandomDirectionInShape(const EmitterConfig& config, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    switch (config.shape) {
    case EmitterShape::Point:
    case EmitterShape::Sphere:
    case EmitterShape::Box: {
        // 均匀随机方向
        float theta = dist01(rng) * glm::two_pi<float>();
        float phi = std::acos(2.0f * dist01(rng) - 1.0f);
        return Vector3(
            std::sin(phi) * std::cos(theta),
            std::cos(phi),
            std::sin(phi) * std::sin(theta)
        );
    }

    case EmitterShape::Cone: {
        // 锥体方向约束：在锥体角度内随机偏转
        float halfAngle = glm::radians(config.coneAngle * 0.5f);
        float theta = dist01(rng) * glm::two_pi<float>();
        float phi = dist01(rng) * halfAngle;
        Vector3 right = glm::normalize(glm::cross(config.forward, Vector3(0.0f, 1.0f, 0.0f)));
        if (glm::length(right) < 0.001f) {
            right = glm::normalize(glm::cross(config.forward, Vector3(1.0f, 0.0f, 0.0f)));
        }
        Vector3 up = glm::normalize(glm::cross(right, config.forward));
        return glm::normalize(
            config.forward * std::cos(phi)
            + right * std::sin(phi) * std::cos(theta)
            + up * std::sin(phi) * std::sin(theta)
        );
    }

    default:
        return Vector3(0.0f, 1.0f, 0.0f);
    }
}

// ============================================================================
// CPUParticleSystem 实现
// ============================================================================

CPUParticleSystem::CPUParticleSystem() {
    m_Particles.reserve(m_Capacity);
}

CPUParticleSystem::~CPUParticleSystem() {
    Clear();
}

void CPUParticleSystem::Update(float dt, const Vector3& cameraPosition) {
    for (auto& p : m_Particles) {
        if (!p.IsAlive()) continue;

        // 老化
        p.age += dt;

        // 生命周期检查
        if (p.age >= p.lifetime) {
            // 标记死亡（由外部查询 IsAlive 处理）
            continue;
        }

        // 应用重力
        // 注意：配置中的重力通过 EmitterConfig 传入
        // 此处 velocity 已被外部根据重力更新

        // 位置积分
        p.position += p.velocity * dt;

        // 旋转积分
        p.rotation += p.angularVelocity * dt;

        // 颜色插值（由外部或渲染管线使用 GetNormalizedAge 完成）
    }

    // 移除死亡粒子（可选：池模式跳过此步以提高性能）
    auto deadEnd = std::remove_if(m_Particles.begin(), m_Particles.end(),
        [](const Particle& p) { return !p.IsAlive(); });
    m_Particles.erase(deadEnd, m_Particles.end());
}

void CPUParticleSystem::Spawn(uint32_t count, const EmitterConfig& config, std::mt19937& rng) {
    uint32_t actualCount = std::min(count, static_cast<uint32_t>(m_Capacity - m_Particles.size()));

    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    // 先分配空间避免反复 reallocate
    size_t needed = m_Particles.size() + actualCount;
    if (needed > m_Particles.capacity()) {
        m_Particles.reserve(std::min(static_cast<size_t>(m_Capacity), needed));
    }

    for (uint32_t i = 0; i < actualCount; ++i) {
        Particle p;

        // 位置
        p.position = RandomPositionInShape(config, rng);

        // 速度方向 + 大小
        Vector3 dir = RandomDirectionInShape(config, rng);
        float speed = config.speed.Random(rng);
        p.velocity = dir * speed;

        // 生命周期
        p.lifetime = config.lifetime.Random(rng);
        p.age = 0.0f;

        // 颜色
        Vector4 startCol = config.startColor.Random(rng);
        p.color = startCol;

        // 尺寸
        p.size = config.startSize.Random(rng);

        // 旋转
        p.rotation = glm::radians(config.rotation.Random(rng));
        p.angularVelocity = glm::radians(config.angularVelocity.Random(rng));

        m_Particles.push_back(p);
    }
}

void CPUParticleSystem::SpawnBurst(const EmitterConfig& config, std::mt19937& rng) {
    Spawn(config.burstCount, config, rng);
}

void CPUParticleSystem::Clear() {
    m_Particles.clear();
}

void CPUParticleSystem::Reset() {
    Clear();
    m_LodFactor = 1.0f;
}

size_t CPUParticleSystem::GetAliveCount() const {
    return std::count_if(m_Particles.begin(), m_Particles.end(),
        [](const Particle& p) { return p.IsAlive(); });
}

void CPUParticleSystem::SortByDistance(const Vector3& cameraPosition) {
    if (!m_SortEnabled) return;

    std::sort(m_Particles.begin(), m_Particles.end(),
        [&cameraPosition](const Particle& a, const Particle& b) {
            float distA = glm::distance2(a.position, cameraPosition);
            float distB = glm::distance2(b.position, cameraPosition);
            return distA > distB; // 远到近排序（远处先绘制）
        });
}

// ============================================================================
// ParticleEmitterComponent 实现
// ============================================================================

ParticleEmitterComponent::ParticleEmitterComponent() {
    std::random_device rd;
    m_RNG.seed(rd());
}

ParticleEmitterComponent::~ParticleEmitterComponent() {
    Shutdown();
}

void ParticleEmitterComponent::Initialize() {
    m_Playing = m_Config.burstCount > 0 || m_Config.spawnRate > 0.0f;
    m_SpawnTimer = 0.0f;
    m_DoBurst = m_Config.burstCount > 0;

    LOG_TRACE("Particles", "发射器初始化: lifetime=[{:.1f},{:.1f}], rate={:.1f}, burst={}",
              m_Config.lifetime.min, m_Config.lifetime.max,
              m_Config.spawnRate, m_Config.burstCount);
}

void ParticleEmitterComponent::Shutdown() {
    m_CPUParticles.Clear();
    m_Playing = false;
}

void ParticleEmitterComponent::Update(Timestep ts) {
    if (!m_Playing && !m_OneShot) return;

    float dt = ts.GetSeconds();
    if (dt <= 0.0f) return;

    // 从场景节点获取世界位置
    auto transform = GetTransform();
    if (transform) {
        m_Config.position = transform->GetPosition();
    }

    // 一次爆发
    if (m_DoBurst) {
        m_CPUParticles.SpawnBurst(m_Config, m_RNG);
        m_DoBurst = false;
        if (m_OneShot) {
            m_Playing = false;
        }
    }

    // 持续发射
    if (m_Playing && m_Config.spawnRate > 0.0f) {
        m_SpawnTimer += dt;

        float spawnInterval = 1.0f / m_Config.spawnRate;
        while (m_SpawnTimer >= spawnInterval) {
            m_SpawnTimer -= spawnInterval;
            m_CPUParticles.Spawn(1, m_Config, m_RNG);

            // 如果超出容量，停止发射
            if (m_CPUParticles.GetTotalCount() >= m_CPUParticles.GetCapacity()) {
                m_SpawnTimer = 0.0f;
                if (m_OneShot) m_Playing = false;
                break;
            }
        }
    }

    // 更新粒子物理
    // 注意：重力在发射器配置中，需要应用到每个粒子的 velocity
    // 此处委托给外部（ParticleSystem::Update），或内部直接更新
    // 粒子物理更新在 Update 中通过 velocity += gravity * dt 完成
    // 但实际上 CPUParticleSystem::Update 由外部每帧调用

    // 检查是否所有粒子死亡（一次爆发且播完）
    if (m_OneShot && m_CPUParticles.GetAliveCount() == 0 && !m_DoBurst) {
        m_Playing = false;
    }

    // 循环模式：当所有粒子死亡且停止发射时重新开始
    if (m_Looping && !m_Playing && m_CPUParticles.GetAliveCount() == 0) {
        m_Playing = true;
        m_DoBurst = m_Config.burstCount > 0;
    }
}

// ============================================================================
// ParticleSystem 实现（子系统）
// ============================================================================

int ParticleSystem::Initialize() {
    LOG_INFO("Particles", "粒子系统正在初始化...");
    m_Emitters.reserve(64);
    LOG_INFO("Particles", "粒子系统初始化完成");
    return 0;
}

void ParticleSystem::Shutdown() {
    LOG_INFO("Particles", "粒子系统正在关闭...");
    ClearAll();
    LOG_INFO("Particles", "粒子系统已关闭");
}

void ParticleSystem::Update(Timestep ts) {
    float dt = ts.GetSeconds();
    if (dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f; // 限制最大步长

    // 逐发射器更新
    for (auto& emitter : m_Emitters) {
        if (!emitter->IsEnabled()) continue;

        // 更新发射器逻辑（生成新粒子等）
        emitter->Update(ts);

        // 更新 CPU 粒子物理
        auto& cpuSystem = emitter->GetParticleSystem();
        Vector3 camPos = emitter->GetWorldPosition(); // 默认使用发射器位置
        cpuSystem.Update(dt, camPos);

        // 透明度排序（如果启用）
        if (cpuSystem.IsSortEnabled()) {
            cpuSystem.SortByDistance(camPos);
        }
    }
}

ParticleEmitterComponent* ParticleSystem::CreateEmitter(const EmitterConfig& config) {
    auto emitter = std::make_unique<ParticleEmitterComponent>();
    emitter->SetConfig(config);
    emitter->Initialize();

    ParticleEmitterComponent* ptr = emitter.get();
    m_Emitters.push_back(std::move(emitter));
    return ptr;
}

void ParticleSystem::DestroyEmitter(ParticleEmitterComponent* emitter) {
    auto it = std::find_if(m_Emitters.begin(), m_Emitters.end(),
        [emitter](const std::unique_ptr<ParticleEmitterComponent>& e) {
            return e.get() == emitter;
        });
    if (it != m_Emitters.end()) {
        (*it)->Shutdown();
        m_Emitters.erase(it);
    }
}

void ParticleSystem::ClearAll() {
    for (auto& emitter : m_Emitters) {
        emitter->Shutdown();
    }
    m_Emitters.clear();
}

} // namespace Prisma::Particles
