#pragma once

#include "RenderTypes.h"
#include <cstdint>

namespace PrismaEngine::Graphic {

/// @brief 围栏抽象接口
/// 用于CPU和GPU之间的同步
class IFence {
public:
    virtual ~IFence() = default;

    /// @brief 获取围栏状态
    /// @return 围栏状态
    virtual FenceState GetState() const = 0;

    /// @brief 获取围栏值
    /// @return 当前围栏值
    virtual uint64_t GetCompletedValue() const = 0;

    /// @brief 信号围栏
    /// @param value 信号值
    virtual void Signal(uint64_t value) = 0;

    /// @brief 等待围栏
    /// @param value 等待的值
    /// @param timeout 超时时间（毫秒），0表示无限等待
    /// @return 是否成功
    virtual bool Wait(uint64_t value, uint64_t timeout = 0) = 0;

    /// @brief 重置围栏
    virtual void Reset() = 0;

    /// @brief 设置事件
    /// @param event 事件句柄
    virtual void SetEventOnCompletion(uint64_t value, void* event) = 0;
};

} // namespace PrismaEngine::Graphic