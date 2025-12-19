#pragma once

#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "interfaces/IFence.h"
#include <wrl/client.h>

#ifdef CreateEvent
#undef CreateEvent
#endif

namespace PrismaEngine::Graphic::DX12 {

/// @brief DirectX12围栏适配器
/// 实现IFence接口，包装ID3D12Fence
class DX12Fence : public IFence {
public:
    /// @brief 构造函数
    /// @param fence D3D12围栏对象
    explicit DX12Fence(Microsoft::WRL::ComPtr<ID3D12Fence> fence);

    /// @brief 析构函数
    ~DX12Fence() override;

    // IFence接口实现
    FenceState GetState() const override;
    uint64_t GetCompletedValue() const override;
    void Signal(uint64_t value) override;
    bool Wait(uint64_t value, uint64_t timeout) override;
    void Reset() override;
    void SetEventOnCompletion(uint64_t value, void* event) override;

    // === DirectX12特定方法 ===

    /// @brief 获取D3D12围栏
    /// @return 围栏指针
    ID3D12Fence* GetFence() const { return m_fence.Get(); }

    /// @brief 等待围栏（使用事件句柄）
    /// @param value 等待的值
    /// @param timeout 超时时间（毫秒）
    /// @return 是否成功
    bool WaitForValue(uint64_t value, uint64_t timeout = INFINITE);

private:
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    uint64_t m_currentValue = 0;
    HANDLE m_event = nullptr;

    // 创建事件句柄
    void CreateEvent();
};

} // namespace PrismaEngine::Graphic::DX12