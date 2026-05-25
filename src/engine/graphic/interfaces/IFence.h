#pragma once

#include "RenderTypes.h"
#include <cstdint>

namespace Prisma::Graphic {

// 围栏抽象接口
/// 用于CPU和GPU之间的同步
class IFence {
public:
    virtual ~IFence() = default;

    // 获取围栏状态
    virtual FenceState GetState() const = 0;

    // 获取围栏值
    virtual uint64_t GetCompletedValue() const = 0;

    // 信号围栏
    virtual void Signal(uint64_t value) = 0;

    // 等待围栏
    virtual bool Wait(uint64_t value, uint64_t timeout = 0) = 0;

    // 重置围栏
    virtual void Reset() = 0;

    // 设置事件
    virtual void SetEventOnCompletion(uint64_t value, void* event) = 0;
};

} // namespace Prisma::Graphic