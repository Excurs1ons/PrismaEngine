#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

// 平台兼容的对齐分配
#ifdef _MSC_VER
#include <malloc.h>
#define PRISMA_ALIGNED_ALLOC(alignment, size) _aligned_malloc(size, alignment)
#define PRISMA_ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
#define PRISMA_ALIGNED_ALLOC(alignment, size) std::aligned_alloc(alignment, size)
#define PRISMA_ALIGNED_FREE(ptr) std::free(ptr)
#endif

namespace Prisma::Memory {

// 分配器类型枚举
enum class MemoryType : uint8_t {
    Unknown     = 0,
    Pool        = 1,
    Frame       = 2,
    Stack       = 3,
    General     = 4,
};

// 分配统计
struct AllocationStats {
    size_t totalAllocated   = 0;  // 已分配总字节数
    size_t totalDeallocated = 0;  // 已释放总字节数
    size_t currentUsed      = 0;  // 当前使用中字节数
    size_t peakUsed         = 0;  // 峰值使用字节数
    size_t allocationCount  = 0;  // 分配次数
    size_t deallocationCount = 0; // 释放次数
};

/// 内存分配器抽象基类
/// 遵循 IAudioDevice 的接口风格：纯虚接口，虚析构
class MemoryAllocator {
public:
    virtual ~MemoryAllocator() = default;

    /// 分配指定大小和对齐的内存块
    /// \param size      请求的字节数
    /// \param alignment 地址对齐要求（必须为 2 的幂）
    /// \return 分配的内存指针，失败返回 nullptr
    [[nodiscard]] virtual void* allocate(size_t size, size_t alignment) = 0;

    /// 释放之前分配的内存
    /// \param ptr allocate() 返回的指针
    virtual void deallocate(void* ptr) = 0;

    /// 重置分配器，释放所有内部状态
    virtual void reset() = 0;

    /// 获取分配器类型
    virtual MemoryType getMemoryType() const = 0;

    /// 获取分配器名称
    virtual const char* getName() const = 0;

    /// 获取分配统计信息（可选实现）
    virtual AllocationStats getAllocationStats() const { return {}; }

    /// 检查是否已初始化
    virtual bool isInitialized() const { return true; }
};

} // namespace Prisma::Memory
