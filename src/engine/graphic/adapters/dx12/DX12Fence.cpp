#include "DX12Fence.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace PrismaEngine::Graphic::DX12 {

DX12Fence::DX12Fence(ComPtr<ID3D12Fence> fence)
    : m_fence(fence)
    , m_currentValue(0)
    , m_event(nullptr) {
    if (fence) {
        m_currentValue = fence->GetCompletedValue();
    }
}

DX12Fence::~DX12Fence() {
    CloseHandle(m_event);
}

// IFence接口实现
FenceState DX12Fence::GetState() const {
    if (!m_fence) {
        return FenceState::Idle;
    }

    uint64_t completedValue = m_fence->GetCompletedValue();
    if (completedValue < m_currentValue) {
        return FenceState::InFlight;
    }
    return FenceState::Completed;
}

uint64_t DX12Fence::GetCompletedValue() const {
    return m_fence ? m_fence->GetCompletedValue() : 0;
}

void DX12Fence::Signal(uint64_t value) {
    if (!m_fence) {
        return;
    }

    m_currentValue = value;

    // 需要通过命令队列来设置信号
    // 这里暂时只更新本地值，实际信号设置需要在RenderDevice中完成
}

bool DX12Fence::Wait(uint64_t value, uint64_t timeout) {
    return WaitForValue(value, timeout);
}

bool DX12Fence::WaitForValue(uint64_t value, uint64_t timeout) {
    if (!m_fence) {
        return true; // 没有fence，认为已就绪
    }

    uint64_t startTime = GetTickCount64();
    uint64_t timeoutMs = timeout;

    // 检查fence是否已达到指定值
    while (m_fence->GetCompletedValue() < value) {
        uint64_t currentTime = GetTickCount64();
        if (timeoutMs != 0 && (currentTime - startTime) >= timeoutMs) {
            return false; // 超时
        }

        // 短暂休眠以避免忙等待
        Sleep(1);
    }

    return true;
}

void DX12Fence::Reset() {
    if (!m_fence) {
        return;
    }

    // 只更新本地值，实际重置需要在RenderDevice中完成
    m_currentValue = 0;
}

void DX12Fence::SetEventOnCompletion(uint64_t value, void* event) {
    if (!m_fence) {
        return;
    }

    if (event) {
        // 设置事件，当fence达到指定值时触发
        HRESULT hr = m_fence->SetEventOnCompletion(value, static_cast<HANDLE>(event));
        if (SUCCEEDED(hr)) {
            m_event = event;
        }
    }
}

// DirectX12特定方法
bool DX12Fence::WaitForValue(uint64_t value, uint64_t timeout) {
    if (!m_fence) {
        return true;
    }

    uint64_t startTime = GetTickCount64();
    uint64_t timeoutMs = timeout;

    // 检查fence值
    uint64_t completedValue = m_fence->GetCompletedValue();
    while (completedValue < value) {
        if (m_event && completedValue < value) {
            // 使用事件等待
            DWORD waitResult = WaitForSingleObject(
                static_cast<HANDLE>(m_event),
                static_cast<DWORD>(timeoutMs > 0 ? timeoutMs : INFINITE)
            );

            if (waitResult == WAIT_OBJECT_0) {
                completedValue = value; // 事件被触发
            } else if (waitResult == WAIT_TIMEOUT) {
                return false; // 超时
            }
        } else {
            // 没有事件，使用忙等待
            uint64_t currentTime = GetTickCount64();
            if (timeoutMs != 0 && (currentTime - startTime) >= timeoutMs) {
                return false;
            }

            // 短暂休眠
            Sleep(1);

            // 更新完成的值
            completedValue = m_fence->GetCompletedValue();
        }
    }

    return true;
}

// 辅助方法
void DX12Fence::CreateEvent() {
    if (!m_event) {
        m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }
}

} // namespace PrismaEngine::Graphic::DX12