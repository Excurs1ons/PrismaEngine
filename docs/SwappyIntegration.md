# Google Swappy 帧率管理集成

> **状态**: 🔲 规划中
> **优先级**: 中
> **依赖**: Vulkan 后端集成完成

## 概述

Google Swappy 是 Android 上用于优化帧率同步的开源库，提供流畅的帧率管理和功耗优化。

## 功能特性

| 功能 | 说明 | 优先级 |
|------|------|--------|
| **帧率同步** | 自动适配屏幕刷新率 (60/90/120Hz) | 高 |
| **Swap 调度** | 优化 SwapChain 呈现时机 | 高 |
| **性能统计** | FPS/帧时间/掉帧统计 | 中 |
| **功耗优化** | 自动降帧以降低功耗 | 中 |
| **Windows 移植** | 参考 Swappy 实现 Windows 版本 | 低 |

## 技术方案

```
src/engine/graphic/
├── PresentScheduler.h        # 呈现调度器抽象
├── SwappyScheduler.h/cpp     # Android Swappy 实现
├── WindowsScheduler.h/cpp    # Windows D3D12 实现
└── FrameStats.h/cpp          # 帧率统计
```

## API 设计

```cpp
namespace Engine::Graphic {

class PresentScheduler {
public:
    static PresentScheduler& getInstance();

    // 初始化
    bool initialize(void* nativeWindow);

    // 帧控制
    void setFrameDuration(int64_t duration_ns);
    void setAutoSwapInterval(bool enable);

    // 呈现
    void present(VkSwapchainKHR swapchain, uint32_t imageIndex);

    // 统计
    FrameStats getFrameStats() const;
};

struct FrameStats {
    float currentFPS;
    float frameTimeMs;
    int droppedFrames;
};

} // namespace
```

## 集成阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | Swappy 库集成 (Android) | ⏳ 计划中 |
| Phase 2 | Vulkan SwapChain 适配 | ⏳ 计划中 |
| Phase 3 | 性能统计面板 | ⏳ 计划中 |
| Phase 4 | Windows 后端实现 | ⏳ 计划中 |

## 参考资料

- [Google Swappy GitHub](https://github.com/google/swappy)
- [Choreographer](https://developer.android.com/reference/android/view/Choreographer)
- [Vulkan Swapchain](https://www.khronos.org/registry/vulkan/specs/1.3/html/chap7.html#_synchronization_and_waiting_presenting_wsi)

---

*文档创建时间: 2025-12-25*
