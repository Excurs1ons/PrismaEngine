#pragma once

#include "core/ISubSystem.h"
#include "MemoryAllocator.h"
#include "MemoryManager.h"

namespace Prisma::Memory {

/// ISubSystem 适配层 — 将 MemoryManager 注册到引擎生命周期
class MemorySystem final : public ISubSystem {
public:
    int Initialize() override;
    void Shutdown() override;
    void Update([[maybe_unused]] Timestep ts) override;
    const char* GetName() const override;

    /// 直接访问底层 MemoryManager
    MemoryManager& GetManager() { return MemoryManager::Get(); }

    /// 获取当前分配统计
    AllocationStats GetStats() const { return MemoryManager::Get().GetAllocationStats(); }
};

} // namespace Prisma::Memory
