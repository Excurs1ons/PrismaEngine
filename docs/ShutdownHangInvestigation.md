# 关闭卡死问题排查报告

## 问题描述

在 2DTemplate (Scene2DTest) 项目中，点击窗口关闭按钮或按 ESC 退出时，引擎卡死在以下日志之后：

```
[INFO] [Engine] (Engine.cpp:277) 正在关闭引擎...
```

## 调查路径

### 阶段一：缩小范围

1. **确认退出流程正常执行到 Shutdown()**：在 `Engine::Run()` 尾部添加日志，确认 `OnShutdown()`、`WaitForIdle()`、`m_CurrentApp.reset()` 均正常完成。

2. **在 Shutdown() 中添加分步日志**：在子系统关闭循环中加入编号日志，定位到卡死在 `MCPSubSystem::Shutdown()`。

3. **在 MCPSubSystem 内添加日志**：确认卡死在 `MCPServer::Stop()` → `TransportStdio::Stop()` → `m_ReadThread.join()`。

### 阶段二：根因分析 — MCP Stdio 线程阻塞

**根因**：`TransportStdio::readLoop()` 在独立的读取线程中执行 `std::getline(std::cin, line)`，这是一个**阻塞式 I/O 调用**。当引擎直接启动（而非通过 MCP client 启动 stdin 管道）时，stdin 没有任何输入，`getline` 永远不会返回。设置 `m_Running = false` 无法唤醒它，因为阻塞发生在操作系统 I/O 层，而非条件变量。

```cpp
// TransportStdio.cpp  —  问题代码
void TransportStdio::readLoop() {
    std::string line;
    while (m_Running && std::getline(std::cin, line)) {  // ← 永远阻塞
        processLine(line);
    }
}
```

**修复**：在 `TransportStdio::Stop()` 中，在 `join()` 之前调用 Windows API `CancelIoEx` 取消 stdin 句柄上的待决读取操作。这将导致 `ReadFile`（`getline` 底层调用）报错返回，`getline` 失败退出循环，线程可以正常 join。

```cpp
void TransportStdio::Stop() {
    m_Running = false;
    if (m_ReadThread.joinable()) {
#ifdef _WIN32
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        if (hStdin != INVALID_HANDLE_VALUE) {
            CancelIoEx(hStdin, nullptr);  // 唤醒阻塞的 getline
        }
#endif
        m_ReadThread.join();
    }
}
```

### 阶段三：Vulkan 资源生命周期问题

MCP 修复后引擎能继续关闭流程，但随后出现 Vulkan 验证层错误和崩溃：

```
[ERROR: Validation] - VUID-vkDestroyDevice-device-05137
VkSampler / VkDescriptorSet / VkDescriptorPool 未被销毁
→ 崩溃于 VulkanSampler::~VulkanSampler() (m_Systems.clear() 中)
```

**根因**：Vulkan 设备销毁时，尚有 GPU 子资源未被释放：

1. **`m_defaultSampler`** — `RenderResourceManager` 缓存了一个 `shared_ptr<ISampler>`，但 `Shutdown()` 中调用的 `ReleaseAllResources()` 只清空了 `m_resources` 映射表，**没有重置 `m_defaultSampler`**。该采样器最终在 `m_Systems.clear()` 销毁 Manager 时才释放，但此时 `vkDestroyDevice` 已经执行完毕，VkDevice 句柄失效，导致 `vkDestroySampler` 崩溃。

2. **`m_descriptorPool`** — `RenderDeviceVulkan::Shutdown()` 中**从未调用 `vkDestroyDescriptorPool`**，描述符池直接泄漏。验证层在 `vkDestroyDevice` 时检测到未销毁的子对象并报错。

**修复**：
```cpp
// RenderResourceManager::Shutdown()  —  在设备销毁前释放默认采样器
m_defaultSampler.reset();

// RenderDeviceVulkan::Shutdown()  —  显式销毁描述符池
vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
```

## 修复的文件汇总

| 文件 | 修复类型 | 说明 |
|------|----------|------|
| `src/engine/mcp/transport/TransportStdio.cpp` | Bug fix | `CancelIoEx` 取消 stdin 阻塞读取 |
| `src/engine/graphic/RenderResourceManager.cpp` | Bug fix | 提前释放 `m_defaultSampler` |
| `src/engine/graphic/adapters/vulkan/RenderDeviceVulkan.cpp` | Bug fix | 补充 `vkDestroyDescriptorPool` |
| `src/engine/app/Engine.cpp` | 调试日志 | 子系统关闭日志 + 类型名 |
| `src/engine/graphic/RenderSystem.cpp` | 调试日志 | 分步关闭日志 |
| `src/engine/graphic/RenderResourceManager.cpp` | 调试日志 | 加载线程 + 资源释放日志 |
| `src/engine/graphic/adapters/vulkan/RenderDeviceVulkan.cpp` | 调试日志 | 逐步骤 Vulkan 关闭日志 |
| `src/engine/window/Window.cpp` | 调试日志 | SDL 窗口销毁日志 |

## 经验教训

1. **阻塞 I/O 在后台线程是危险的**：`std::getline(std::cin, ...)` 在后台线程执行时，退出时没有可靠的跨平台唤醒方式。对于类似场景应考虑：
   - 使用非阻塞 I/O + select/poll 定时检查
   - 使用自管道的 self-pipe trick（Linux）
   - 使用 `CancelIoEx`（Windows）
2. **资源释放顺序至关重要**：Vulkan 等显式 GPU API 要求所有子对象在父对象销毁前释放。shared_ptr 延迟释放可能导致子对象在父对象销毁后仍然存活，造成崩溃。
3. **调试日志的价值**：在关闭路径中添加分步日志可以将"程序卡死"这种模糊问题精确到具体子系统和函数。
