# 引擎配置与性能优化

## 1. 引擎级开关集中化

所有渲染相关的硬编码开关集中在 `src/engine/app/Engine.cpp` 的 `RenderSystemDesc` 初始化处：

```cpp
// Engine.cpp:140-147
Graphic::RenderSystemDesc rDesc;
rDesc.windowHandle = m_Window->GetNativeWindow();
rDesc.width = m_Window->GetWidth();
rDesc.height = m_Window->GetHeight();
// === 引擎级开关（后续可改为配置文件） ===
rDesc.enableDebug      = false;       // 调试输出
rDesc.enableVSync      = false;       // 垂直同步
rDesc.enableValidation = false;       // Vulkan 验证层（开启后大幅降低性能）
```

### 开关说明

| 开关 | 对应文件 | 作用 |
|------|---------|------|
| `enableDebug` | `RenderSystem.cpp` 各日志 | 调试路径，Release 关闭 |
| `enableVSync` | `VulkanSwapChain.cpp:102` | 控制选择 FIFO 或 IMMEDIATE present mode |
| `enableValidation` | `RenderSystem.cpp:56-61` | 控制 Vulkan 验证层加载（原 `#ifdef _DEBUG` 宏已移除） |

### 移除的散落硬编码

- **`RenderSystem.cpp`** — 原有的 `#ifdef _DEBUG` / `devDesc.enableValidation = true` 双重控制已移除，统一使用 `rDesc.enableValidation`
- **`VulkanSwapChain.cpp::Resize()`** — 之前写死 `vsync=true`，改为 `m_mode == SwapChainMode::VSync`

## 2. 垂直同步控制链

```
Engine.cpp           rDesc.enableVSync = false
  ↓
RenderSystem.cpp     devDesc.vsync = m_desc.enableVSync
  ↓
RenderDeviceVulkan   m_swapChain->Initialize(surface, w, h, desc.vsync)
  ↓
VulkanSwapChain      set_desired_present_mode(vsync ? FIFO : IMMEDIATE)
```

### Present Mode 选择

| `enableVSync` | Vulkan Present Mode | 行为 |
|:---:|:---:|---|
| `true` | `VK_PRESENT_MODE_FIFO_KHR` | 垂直同步，帧率锁定显示器刷新率 |
| `false` | `VK_PRESENT_MODE_IMMEDIATE_KHR` | 不等待垂直消隐，无上限帧率 |

### 三缓冲

```cpp
.set_desired_min_image_count(3)  // VulkanSwapChain.cpp:103
```

所有模式下均请求至少 3 个交换链图像，确保 `vkAcquireNextImageKHR` 始终有可用图像。

## 3. 帧率限制分析

### 三阶段瓶颈

| 阶段 | 耗时 (Debug验证层ON) | 耗时 (验证层OFF) | 瓶颈类型 |
|------|:---:|:---:|---|
| 应用渲染 (`Render`) | 1.34ms | 1.34ms | CPU — 1751 次 DrawQuad 调用 |
| GPU 提交 (`EndFrame/Pipeline`) | 6.86ms | 1.72ms | 驱动 — 1751 次独立 DrawIndexed |
| 交换链 (`BeginFrame/Present`) | 0.07ms | 0.01ms | GPU — 几乎无开销 |
| **合计** | **~8.3ms (120FPS)** | **~3.1ms (320FPS)** | |

### 当前瓶颈

验证层关闭后帧时间分解（`@ 320 FPS`）：

```
         BF=0.01ms     Render=1.34ms     EF=1.72ms     Present=0.06ms
CPU:   [  fence wait ] [ 绘制命令生产 ] [ 排序+提交  ] [ 图像呈现  ]
        ◄──────────────  3.1ms CPU 瓶颈 ——————————————►
GPU:                                                    [ <0.1ms 渲染 ]
```

- **CPU 时间** = 3.1ms/帧（瓶颈）
- **GPU 时间** = <0.1ms/帧（5070Ti利用率 ~2%）
- **实际帧率** = 320 FPS（CPU 上限）
- **理论帧率** = 显卡性能未发挥

### `vkAcquireNextImageKHR` 超时策略

```cpp
// VulkanSwapChain.cpp:218-225
// 使用 UINT64_MAX 等待图像可用。
// 在 Immediate 模式下，这通常会立即返回；在 FIFO (VSync) 模式下，这会阻塞直到垂直同步。
// 之前的 timeout=0 会导致在图像未立即准备好时跳过整帧渲染，反而限制了表现。
VkResult result = vkAcquireNextImageKHR(..., UINT64_MAX, ...);
```

- `UINT64_MAX`（阻塞等待）：配合三缓冲 + IMMEDIATE 模式，实际几乎不阻塞
- `timeout=0`（非阻塞）：在图像未准备好时返回 `VK_NOT_READY`，导致整帧跳过
- 非阻塞模式看似能"绕过"显示器刷新率限制，但实际上帧跳过会大幅降低平均帧率

## 4. CPU 瓶颈根因：逐 Quad 绘制

### 架构问题

每个 `Renderer2D::DrawQuad()` 调用流程：

```
DrawQuad()
  → Renderer::Submit(mesh, material, transform, color)    // 入队命令
  → OpaquePass::Execute()
      → for each command:
          → material->Bind(cmd)                           // 绑定描述符集
          → PushConstants(MVP, Color)                     // 推送常量
          → SetVertexBuffer / SetIndexBuffer              // 设置缓冲区
          → DrawIndexed(6)                                // 6 个索引/Quad
```

1751 个 Quad = 1751 次 `DrawIndexed` 调用。每次调用都有 Vulkan 驱动开销。

### 优化方向

要实现 GPU 满负荷运行，需要将 1751 个独立 Quad 合批为 1 次 `DrawIndexed` 调用：

| 方案 | 改动范围 | 预期效果 |
|------|---------|---------|
| **Renderer2D 内部合批**：攒 quad 顶点数据到一个大 buffer，Flush 时一次提交 | `Renderer2D.cpp` 内的 `DrawQuad`/`Flush`/`StartBatch` | 驱动调用从 1751 → 1，CPU 耗时从 3ms → <0.2ms |
| **GPU Instancing**：一次 DrawIndexedInstanced 绘制所有 Quad | `RenderCommand` 或 `Submit` 接口 | 更进一步减少 CPU 开销 |
| **Indirect Drawing**：GPU 自主决定绘制内容 | 更彻底的架构改造 | CPU 几乎零开销 |

### 当前合批状态

`Renderer2D` 的 `StartBatch()/NextBatch()` 已预留合批接口，但 `DrawQuad()` 当前仍逐个调用 `Renderer::Submit()`，未实际合并顶点数据。`StartBatch()` 仅重置统计计数，`NextBatch()` 仅调用 `Flush() + StartBatch()`。
