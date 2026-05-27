#pragma once

#include "core/Component.h"
#include "CPUParticleSystem.h"
#include "Export.h"
#include <memory>

namespace Prisma::Particles {

// 粒子发射器组件 — 附加到场景 Node 上的发射器
class ENGINE_API ParticleEmitterComponent : public Component {
public:
    ParticleEmitterComponent();
    ~ParticleEmitterComponent() override;

    // Component 接口
    void Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

    // 发射器配置
    EmitterConfig&       GetConfig()       { return m_Config; }
    const EmitterConfig& GetConfig() const { return m_Config; }
    void SetConfig(const EmitterConfig& config) { m_Config = config; }

    // CPU 粒子系统引用
    CPUParticleSystem&       GetParticleSystem()       { return m_CPUParticles; }
    const CPUParticleSystem& GetParticleSystem() const { return m_CPUParticles; }

    // 播放控制
    void Play()   { m_Playing = true; }
    void Stop()   { m_Playing = false; }
    void Burst()  { if (m_Playing) m_DoBurst = true; }
    bool IsPlaying() const { return m_Playing; }

    // 单次爆发（一次 Burst 后自动停止）
    void SetOneShot(bool oneShot) { m_OneShot = oneShot; }
    bool IsOneShot() const { return m_OneShot; }

    // 循环
    void SetLooping(bool looping) { m_Looping = looping; }
    bool IsLooping() const { return m_Looping; }

    // 发射器世界位置（由 Scene 更新）
    void SetWorldPosition(const Vector3& pos) { m_WorldPosition = pos; }
    const Vector3& GetWorldPosition() const { return m_WorldPosition; }

    // 组件标识
    ComponentId GetComponentId() const override { return GetComponentTypeId<ParticleEmitterComponent>(); }
    const char* GetComponentTypeName() const override { return "ParticleEmitter"; }

private:
    EmitterConfig    m_Config;
    CPUParticleSystem m_CPUParticles;
    Vector3          m_WorldPosition = Vector3(0.0f);
    bool             m_Playing       = true;
    bool             m_DoBurst       = false;
    bool             m_OneShot       = false;
    bool             m_Looping       = true;
    float            m_SpawnTimer   = 0.0f;
    std::mt19937     m_RNG;
};

} // namespace Prisma::Particles
