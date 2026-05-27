#pragma once

#include "ISubSystem.h"
#include "CPUParticleSystem.h"
#include "ParticleEmitterComponent.h"
#include "Export.h"
#include <memory>
#include <vector>

namespace Prisma::Particles {

// 粒子系统 — 子系统包装器，管理所有活跃的粒子发射器
class ENGINE_API ParticleSystem : public ISubSystem {
public:
    ParticleSystem() = default;
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // ========== 子系统接口 ==========
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "ParticleSystem"; }

    // ========== 发射器管理 ==========

    // 创建发射器并返回裸指针（系统持有所有权）
    ParticleEmitterComponent* CreateEmitter(const EmitterConfig& config);

    // 销毁指定的发射器
    void DestroyEmitter(ParticleEmitterComponent* emitter);

    // 获取所有发射器
    const std::vector<std::unique_ptr<ParticleEmitterComponent>>& GetEmitters() const { return m_Emitters; }

    // 获取活跃发射器数量
    size_t GetEmitterCount() const { return m_Emitters.size(); }

    // 清空所有发射器
    void ClearAll();

private:
    std::vector<std::unique_ptr<ParticleEmitterComponent>> m_Emitters;
};

} // namespace Prisma::Particles
