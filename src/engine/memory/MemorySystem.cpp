#include "MemorySystem.h"

namespace Prisma::Memory {

int MemorySystem::Initialize() {
    // MemoryManager 是惰性单例，首次 Get() 即初始化默认分配器
    // 此处可预留预热逻辑
    auto& mgr = MemoryManager::Get();
    (void)mgr;
    return 0;
}

void MemorySystem::Shutdown() {
    MemoryManager::Get().ResetAll();
}

void MemorySystem::Update([[maybe_unused]] Timestep ts) {
    // 帧分配器在每个 ISubSystem::Update 中不需要自动 swap
    // 引擎主循环在合适时机调用 m_FrameAllocator->swapBuffers()
}

const char* MemorySystem::GetName() const {
    return "MemorySystem";
}

} // namespace Prisma::Memory
