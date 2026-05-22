# Prisma Engine 设备配置指南

## 概述

Prisma Engine 支持多个音频和渲染设备，可以通过 CMake 选项在编译时选择启用的设备。

## 音频设备

### 可用设备

- **XAudio2** (Windows) - 高性能，低延迟音频
- **OpenAL** (跨平台) - 专业 3D 音频 API
- **SDL3 Audio** (跨平台) - 简单易用的音频 API

### 配置选项

```cmake
# CMake 命令行配置
cmake .. -DPRISMA_ENABLE_AUDIO_XAUDIO2=ON \
          -DPRISMA_ENABLE_AUDIO_OPENAL=OFF \
          -DPRISMA_ENABLE_AUDIO_SDL3=OFF

# 或者使用 CMake GUI
# 1. 打开 CMake GUI
# 2. 配置项目
# 3. 勾选需要的设备选项
# 4. 点击 Configure 和 Generate
```

### 默认配置

- **Windows**: 仅启用 XAudio2
- **其他平台**: 需要手动启用至少一个设备

## 渲染设备

### 可用设备

- **DirectX 12** (Windows) - 现代 Windows 图形 API
- **OpenGL** (跨平台) - 传统图形 API (4.6+)
- **Vulkan** (跨平台) - 新一代跨平台图形 API (1.3+)
- **Metal** (macOS/iOS) - Apple 平台原生图形 API
- **WebGPU** (Web) - Web 平台图形 API

### 配置选项

```cmake
# CMake 命令行配置
cmake .. -DPRISMA_ENABLE_RENDER_DX12=ON \
          -DPRISMA_ENABLE_RENDER_OPENGL=OFF \
          -DPRISMA_ENABLE_RENDER_VULKAN=OFF

# 启用多个设备（运行时选择）
cmake .. -DPRISMA_ENABLE_RENDER_DX12=ON \
          -DPRISMA_ENABLE_RENDER_VULKAN=ON
```

### 默认配置

- **Windows**: 仅启用 DirectX 12
- **macOS/iOS**: 需要手动启用（推荐 Metal）
- **Linux**: 需要手动启用（推荐 Vulkan 或 OpenGL）
- **Web**: 自动启用 WebGPU

## C# 脚本后端

### 可用后端

- **OFF** (关闭) — 不编译脚本子系统代码，不初始化运行时，不尝试加载 DLL，仅使用 Native 逻辑
- **Mono** — Mono 运行时（预留，暂未实现完整初始化链路）
- **CoreCLR** — .NET CoreCLR 宿主（默认），通过 `hostfxr` 自承载 .NET 运行时

### 编译时配置 (CMake)

```cmake
# CMake 命令行配置
cmake .. -DPRISMA_ENABLE_SCRIPTING=CORECLR    # .NET CoreCLR（默认）
cmake .. -DPRISMA_ENABLE_SCRIPTING=MONO        # Mono 运行时
cmake .. -DPRISMA_ENABLE_SCRIPTING=OFF         # 关闭 C# 脚本
```

### 运行时配置 (project.json)

即使编译时启用了脚本后端，也可以通过 `project.json` 在项目级别控制脚本是否激活：

```json
{
    "name": "MyGame",
    "scriptingBackend": "CoreCLR",
    "window": { ... }
}
```

取值：`"Off"` / `"Mono"` / `"CoreCLR"`（默认 `"CoreCLR"`）。

运行时行为：
- **Off**: 跳过脚本子系统初始化，不加载任何 DLL
- **Mono**: 尝试初始化 Mono 运行时（需编译时启用 `PRISMA_ENABLE_SCRIPTING=MONO`）
- **CoreCLR**: 从 `scripts/` 目录加载 `hostfxr` 和 .NET 运行时（需编译时启用 `PRISMA_ENABLE_SCRIPTING=CORECLR`）

如果运行时选择的后端与编译时不匹配，引擎会输出警告并跳过初始化。

### 默认配置

- **所有平台**: CoreCLR（需要执行 `dotnet publish --self-contained` 生成脚本程序集）

## 高级功能

### 音频功能

```cmake
# 启用 3D 音频
cmake .. -DPRISMA_ENABLE_AUDIO_3D=ON

# 启用音频流式播放
cmake .. -DPRISMA_ENABLE_AUDIO_STREAMING=ON

# 启用音效处理 (EAX/EFX)
cmake .. -DPRISMA_ENABLE_AUDIO_EFFECTS=ON

# 启用 HRTF (双耳音频)
cmake .. -DPRISMA_ENABLE_AUDIO_HRTF=ON
```

### 渲染功能

```cmake
# 启用光线追踪
cmake .. -DPRISMA_ENABLE_RAYTRACING=ON

# 启用网格着色器
cmake .. -DPRISMA_ENABLE_MESH_SHADERS=ON

# 启用可变速率着色
cmake .. -DPRISMA_ENABLE_VARIABLE_RATE_SHADING=ON

# 启用无绑定资源
cmake .. -DPRISMA_ENABLE_BINDLESS_RESOURCES=ON
```

## 平台特定注意事项

### Windows

```cmake
# 推荐配置（最佳性能）
cmake .. -DPRISMA_ENABLE_AUDIO_XAUDIO2=ON \
          -DPRISMA_ENABLE_RENDER_DX12=ON \
          -DPRISMA_ENABLE_RAYTRACING=ON
```

### macOS

```cmake
# 使用 Metal（推荐）
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_METAL=ON

# 或使用跨平台选项
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_OPENGL=ON
```

### Linux

```cmake
# 使用 Vulkan（推荐）
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_VULKAN=ON

# 或使用 OpenGL
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_OPENGL=ON
```

### Android

```cmake
# Android 配置
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_VULKAN=ON
```

### Web (Emscripten)

```cmake
# Web 配置（Runtime + Game 静态链接）
emcmake cmake .. -DPRISMA_BUILD_EDITOR=OFF \
                  -DPRISMA_BUILD_SHARED_LIBS=OFF \
                  -DPRISMA_LAUNCHER_DYNAMIC_LOAD=OFF \
                  -DPRISMA_ENABLE_RENDER_OPENGL=ON
```

## 运行时设备选择

如果编译时启用了多个设备，可以在运行时选择：

```cpp
// 音频
AudioDesc audioDesc;
audioDesc.backendType = AudioDeviceType::OpenAL; // 或 XAudio2, SDL3

// 渲染
RenderSystemDesc renderDesc;
renderDesc.backendType = RenderDeviceType::Vulkan; // 或 DirectX12, OpenGL
```

## 故障排除

### 找不到依赖

如果启用某个后端但找不到相应的库：

1. **Windows (XAudio2)**:
   - Windows SDK 已包含，无需额外安装

2. **OpenAL**:
   - Windows: 下载 [OpenAL Soft](https://openal-soft.org/)
   - Linux: `sudo apt install libopenal-dev`
   - macOS: `brew install openal-soft`

3. **SDL3**:
   - 从 [SDL官网](https://www.libsdl.org/) 下载开发版本

4. **Vulkan**:
   - 安装 [Vulkan SDK](https://vulkan.lunarg.com/)

### 平台不兼容错误

如果看到平台不兼容错误，检查：
- Windows 只能使用 XAudio2 和 DirectX12
- macOS/iOS 只能使用 Metal
- Web 建议使用 OpenGL(ES) / SDL3 组合

### 性能建议

1. **Windows**: 使用 XAudio2 + DirectX12 组合获得最佳性能
2. **跨平台开发**: 使用 OpenAL + Vulkan 作为统一方案
3. **移动设备**: 考虑使用专门的移动端后端优化

### 控制台输出乱码 (Garbled Console Output)

在中文 Windows 环境下，MSVC 和 MSBuild 默认使用 GBK 编码，而现代终端倾向于 UTF-8，这会导致输出乱码。

**推荐解决方法：**
在执行构建前，通过环境变量强制工具链输出英文，这是最稳妥的方案：

```powershell
# PowerShell
$env:VSLANG = '1033'                 # 强制 MSVC 使用英文
$env:DOTNET_CLI_UI_LANGUAGE = 'en-US' # 强制 .NET 使用英文
chcp 437                              # 切换控制台到美国英语代码页
./scripts/build-windows.bat
```

或者使用 UTF-8 强制模式：
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8
$env:DOTNET_CLI_FORCE_UTF8_ENCODING = 'true'
chcp 65001
```

## 配置文件示例

创建 `CMakePresets.json` 快速配置：

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "Windows-Release",
      "generator": "Visual Studio 18 2026",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "PRISMA_ENABLE_AUDIO_XAUDIO2": "ON",
        "PRISMA_ENABLE_RENDER_DX12": "ON",
        "PRISMA_ENABLE_RAYTRACING": "ON"
      }
    },
    {
      "name": "Linux-Release",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "PRISMA_ENABLE_AUDIO_OPENAL": "ON",
        "PRISMA_ENABLE_RENDER_VULKAN": "ON"
      }
    }
  ]
}
```
---

## 引擎级配置开关 / Engine-Level Config Switches
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
