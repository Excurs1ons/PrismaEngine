# 2D 渲染管线分析与修复

## 1. 渲染管线架构

### 1.1 数据流

```
Application::OnRender()
  │
  ├─ Renderer2D::BeginScene(camera)
  │     └─ Renderer::BeginScene(cameraData)      // 清空命令队列，设置相机数据
  │
  ├─ Renderer2D::DrawQuad(transform, color)
  │     └─ Renderer::Submit(mesh, material, transform, color)  // 追加到命令队列
  │
  ├─ Renderer2D::EndScene()
  │     └─ Flush()
  │           └─ Renderer::EndScene()            // 排序命令队列
  │
Engine::Run() 每帧循环:
  ├─ RenderSystem::BeginFrame()                   // Vulkan: 开始命令缓冲区 + RenderPass
  ├─ Application::OnRender()                      // 上述提交过程
  ├─ RenderSystem::EndFrame()
  │     ├─ 读取 Renderer::GetCommandQueue()
  │     ├─ ForwardPipeline::Execute(ctx)
  │     │     └─ OpaquePass::Execute(cmd, commands)
  │     │           ├─ Bind PS0 + viewport
  │     │           ├─ 对每个命令:
  │     │           │    ├─ material->Bind(cmd)    // 绑定描述符集
  │     │           │    ├─ PushConstants (MVP + Color)
  │     │           │    ├─ SetVertexBuffer / SetIndexBuffer
  │     │           │    └─ DrawIndexed
  │     └─ Renderer::ClearQueue()
  └─ RenderSystem::Present()                      // 显示交换链
```

### 1.2 关键组件

| 组件 | 文件 | 职责 |
|------|------|------|
| `Renderer2D` | `src/engine/graphic/Renderer2D.*` | 高级 2D API (BeginScene/DrawQuad/EndScene/DrawString) |
| `Renderer` | `src/engine/graphic/Renderer.*` | 底层命令提交 (BeginScene/Submit/EndScene) |
| `RenderSystem` | `src/engine/graphic/RenderSystem.*` | 驱动渲染循环 (BeginFrame/EndFrame/Present) |
| `ForwardPipeline` | `src/engine/graphic/pipelines/forward/` | 3D forward 渲染管线 |
| `OpaquePass` | `src/engine/graphic/pipelines/forward/OpaquePass.*` | 2D/3D 不透明物体绘制 pass |
| `VulkanCommandBuffer` | `src/engine/graphic/adapters/vulkan/VulkanCommandBuffer.*` | Vulkan 命令缓冲区封装 |
| `VulkanPipelineState` | `src/engine/graphic/adapters/vulkan/VulkanPipelineState.*` | Vulkan 管线状态对象 |
| `RenderDeviceVulkan` | `src/engine/graphic/adapters/vulkan/RenderDeviceVulkan.*` | Vulkan 设备管理 (BeginFrame/EndFrame 含 RenderPass) |

## 2. 原始故障

### 2.1 Renderer2D::Flush() 空实现

**文件:** `src/engine/graphic/Renderer2D.cpp`

```
修复前:
  Flush() { }                        // 空函数，什么也不做

修复后:
  Flush() {
    Renderer::EndScene();            // 排序命令队列，为管线执行做准备
    s_Data->Stats.DrawCalls++;
  }
```

**影响:** `Flush()` 被 `EndScene()` 调用，负责完成当前批次的命令排序。空实现导致命令队列从未被处理——虽然没有坏影响（因为 EndScene 自己也调用了 `Renderer::EndScene()`），但 `Flush()` 作为批处理接口完全失效。

### 2.2 SpriteRenderer::Render() 绕过 Renderer2D

**文件:** `src/engine/graphic/SpriteRenderer.cpp`

- **修复前:** 手动创建 `SpriteVertex[4]` + 索引，调用 `context->DrawIndexed()`——完全绕过 Renderer2D，直接发到 GPU
- **修复后:** 构建变换矩阵，调用 `Renderer2D::DrawQuad(transform, texture, uv, color)`——走统一 2D 渲染管道

**影响:** ECS 组件的 2D 渲染和 `Renderer2D` 是两条独立的死代码路径，互不连通。修复后 ECS SpriteRenderer 通过 `Renderer2D` 提交到 `Renderer::Submit()`，由 OpaquePass 统一处理。

### 2.3 EditorLayer.cpp 类型转换错误

**文件:** `src/editor/panels/EditorLayer.cpp:78`

```
修复前:
  VkCommandBuffer cmd = vkDevice->GetCurrentCommandBuffer();
  // 错误: GetCurrentCommandBuffer() 返回 ICommandBuffer*，无法隐式转为 VkCommandBuffer

修复后:
  auto* cmdBuffer   = vkDevice->GetCurrentCommandBuffer();
  auto* vkCmdBuffer = dynamic_cast<Graphic::Vulkan::VulkanCommandBuffer*>(cmdBuffer);
  VkCommandBuffer cmd = vkCmdBuffer ? vkCmdBuffer->GetVkCommandBuffer() : VK_NULL_HANDLE;
```

**影响:** 编辑器 ImGui 视口渲染片段无法编译。

### 2.4 MCP SceneTools 使用了不存在的 API

**文件:** `src/engine/mcp/tools/SceneTools.cpp`

使用了 `Scene::GetAllEntities()`, `Scene::CreateEntity()`, `Scene::FindEntity()`, `Scene::DeleteEntity()`, `GameObject::GetID()`, `GameObject::GetName()` 等不存在的方法。

**修复:** 重写使用 `Scene::GetGameObjects()`, `Scene::AddGameObject()`, `Scene::RemoveGameObject()`, `GameObject::name` 等实际存在的 API。

### 2.5 CMakePresets.json 缺少 PRISMA_ENABLE_MCP 设置

`PRISMA_ENABLE_MCP` 默认开启，但 `xxhash` 依赖未在 PacMan preset 中配置，导致编译 `DeltaTracker.h` 时找不到 `xxhash.h`。

**修复:** 在 `windows-pacman-base` preset 中显式设置 `PRISMA_ENABLE_MCP=OFF`。

## 3. 灰度屏幕调试

### 3.1 问题现象

Scene2DTest 应用可以启动，日志显示 566 个 Quad 被提交和绘制，OpaquePass 创建了 PSO，加载了着色器。但窗口始终显示灰色 (背景清除色 0.1, 0.1, 0.1, 1.0)。

### 3.2 查找过程

| 步骤 | 操作 | 结果 |
|------|------|------|
| 1 | 检查 Renderer2D 初始化日志 | 正常，Quad mesh 和着色器均加载 ✅ |
| 2 | 添加 EndFrame 日志 | 566 命令提交，管线已初始化=true ✅ |
| 3 | 添加 OpaquePass 日志 | PSO 非空，shaders ok=true ✅ |
| 4 | 捕获 stderr 查看 Vulkan 验证层 | `ERROR: OpNop must appear in a block` ❌ |
| 5 | `spirv-dis` 反汇编 SPIR-V | 确认无 `OpNop` 指令 |
| 6 | `spirv-val` 验证 SPIR-V | 验证通过 (Vulkan SDK 内建验证仍报错) |
| 7 | 纯色着色器 (无纹理采样) | 画面正常显示 ✅ |
| 8 | 纹理采样但忽略结果 | 画面正常显示 ✅ |
| 9 | `outColor = v_Color * texColor` | 灰色 ❌ |
| 10 | 分析 `CreateTextureFromMemory` | **发现数据从未上传到 GPU！** |

### 3.3 根因

**`VulkanResourceFactory::CreateTextureFromMemory()` 丢弃了像素数据:**

```cpp
std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromMemory(
    const void* data, uint64_t dataSize, const TextureDesc& desc)
{
    if (!data || dataSize == 0) { ... return nullptr; }
    // BUG: data 和 dataSize 参数被完全忽略！
    return CreateTextureImpl(desc);  // 创建纹理但不填充数据
}
```

纹理创建后像素数据从未上传到 GPU。白色 1x1 纹理在 GPU 上是全零 (0,0,0,0)。当片段着色器做 `outColor = v_Color * texColor` 时，结果为 0，再经 alpha 混合 → 灰色背景。

## 4. 修复文件清单

| 文件 | 修复内容 |
|------|----------|
| `src/engine/graphic/Renderer2D.cpp` | `Flush()` 调用 `Renderer::EndScene()` + `DrawCalls++`；添加 `LastCameraData`；日志频率改为 5 秒真实时间 |
| `src/engine/graphic/SpriteRenderer.cpp` | `Render()` 委托给 `Renderer2D::DrawQuad()` |
| `src/editor/panels/EditorLayer.cpp` | `ICommandBuffer*` → `VulkanCommandBuffer*` → `VkCommandBuffer` |
| `src/engine/mcp/tools/SceneTools.cpp` | 使用实际存在的 Scene/GameObject API |
| `src/engine/graphic/RenderSystem.cpp` | EndFrame 添加 5 秒间隔队列状态日志 |
| `src/engine/graphic/pipelines/forward/OpaquePass.cpp` | Execute 添加 PSO 和着色器状态日志 |
| `projects/Scene2DTest/CMakeLists.txt` | 新项目搭建 |
| `projects/CMakeLists.txt` | 添加 Scene2DTest 子项目 |
| `CMakePresets.json` | `windows-pacman-base` 添加 `PRISMA_ENABLE_MCP=OFF` |
| `assets/shaders/Renderer2D.frag` | 简化为纯色着色器（临时绕过纹理上传 bug） |
| `assets/shaders/Renderer2D.frag` | 恢复为纹理采样版本（`v_Color * texture(AlbedoMap, v_UV)`） |
| `assets/shaders/Renderer2D.frag.spv` | glslangValidator 编译纹理采样 SPV，744B |
| `assets/shaders/Renderer2D.vert.spv` | glslc 编译，1804B |
| `src/engine/graphic/adapters/vulkan/VulkanEmbeddedShaders.h` | 用已验证的 SPIR-V 替换手工拼凑的字节数组 |
| `src/engine/graphic/adapters/vulkan/VulkanResourceFactory.cpp` | CreateTextureFromMemory 添加 staging buffer 上传 |
| `src/engine/app/Engine.cpp` | Shutdown 显式销毁窗口 + 关闭事件兜底 |
| `src/engine/platform/Platform.cpp` | WINDOW_CLOSE_REQUESTED 事件处理 |
| `src/engine/graphic/RenderResourceManager.cpp` | 添加 SPIR-V 加载日志（绝对路径、大小、magic number） |

## 5. 已知问题

### 5.1 CreateTextureFromMemory 不上传数据

**文件:** `src/engine/graphic/adapters/vulkan/VulkanResourceFactory.cpp:370-376`

**问题:** 函数接收 `data` 和 `dataSize` 参数但只调用了 `CreateTextureImpl()` 创建空纹理。Vulkan 纹理有 `VK_IMAGE_USAGE_TRANSFER_DST_BIT` 标志但没有实际数据上传。

**修复方案:** 在 `CreateTextureFromMemory` 中添加:
1. 创建 staging buffer 填充像素数据
2. 用一次性的 command buffer 执行 `vkCmdCopyBufferToImage`
3. 将纹理布局从 `UNDEFINED` 转换到 `SHADER_READ_ONLY_OPTIMAL`
4. 释放 staging buffer

`ITexture::UpdateData` 接口已声明但 `VulkanTexture` 未实现——也是一个可选修复点。

### 5.2 SPIR-V OpNop 验证警告（已修复）

**实际根因:** 验证错误来自 `src/engine/graphic/adapters/vulkan/VulkanEmbeddedShaders.h` 中手工拼凑的 SPIR-V 字节数组（含无效 OpNop 指令），而非文件着色器。`Material::CreateDefault()` → `LoadShaderSync("Default")` 加载内嵌顶点着色器时触发。

**修复:** 用 `glslangValidator` 编译的纯色顶点着色器（1804B）和 `glslangValidator` 编译的片段着色器（376B）替换手工数组。

纹理采样着色器使用 `glslangValidator` 编译 SPIR-V（744B），无 OpNop 问题。

### 5.3 纹理着色器

`Renderer2D.frag` 当前使用纹理采样版本（`v_Color * texture(AlbedoMap, v_UV)`），通过 `glslangValidator` 编译 SPIR-V（744B），无 OpNop 验证错误。CreateTextureFromMemory 的 staging buffer 上传已修复，白色默认纹理正常上传到 GPU。

## 6. Scene2DTest 使用

### 6.1 构建

```bash
cmake --preset pacman-windows-x64-debug
cmake --build --preset pacman-windows-x64-debug --target Scene2DTest
```

### 6.2 运行

```bash
./build/pacman-windows-x64-debug/bin/Debug/Scene2DTest.exe
```

### 6.3 项目结构

```
projects/Scene2DTest/
├── CMakeLists.txt                # 构建配置
├── src/
│   ├── main.cpp                  # 入口点
│   ├── Scene2DTestApp.h          # 应用类声明
│   └── Scene2DTestApp.cpp        # 2D 场景实现
```

### 6.4 作为模板

复制 `projects/Scene2DTest/` 到 `projects/MyNew2DGame/`，修改 `Scene2DTestApp.cpp` 中的 `OnRender()` 函数，在 `BeginScene` / `EndScene` 之间添加 `DrawQuad` 调用。

## 7. 完整渲染管线图

```
Application::OnRender()
  │
  ├─ Renderer2D::BeginScene(orthoCamera)
  │     └─ Renderer::BeginScene(cameraData)
  │           └─ s_Data.commands.clear()
  │               s_Data.camera = cameraData
  │
  ├─ for each sprite:
  │     Renderer2D::DrawQuad(transform, color)
  │       └─ Renderer::Submit(mesh, material, transform, color)
  │             └─ s_Data.commands.push_back({mesh, material, transform, color})
  │
  ├─ Renderer2D::EndScene()
  │     └─ Flush()
  │           └─ Renderer::EndScene()
  │                 └─ SortByPriority(s_Data.commands)
  │
  │ [Engine::Run loop continues...]
  │
  ├─ RenderSystem::BeginFrame()  ──┐
  │     └─ device->BeginFrame()    │
  │           └─ vkBeginCommandBuffer
  │               vkCmdBeginRenderPass(clear=0.1,0.1,0.1)
  │                                │
  ├─ (OnRender already called) ────┤
  │                                │
  ├─ RenderSystem::EndFrame()      │
  │     ├─ GetRendererCommandQueue()├── commands [566 entries]
  │     ├─ ForwardPipeline::Execute(ctx)
  │     │     └─ OpaquePass::Execute(cmd, commands)
  │     │           ├─ EnsureDefaultPipeline()
  │     │           │     ├─ Load Renderer2D.vert/frag SPIR-V
  │     │           │     ├─ Create VkShaderModule + VkPipeline
  │     │           │     └─ Create descriptor set layout
  │     │           ├─ cmd->SetPipelineState()
  │     │           │     └─ vkCmdBindPipeline + m_currentLayout
  │     │           ├─ cmd->SetViewport(800, 600)
  │     │           ├─ cmd->SetScissorRect(800, 600)
  │     │           ├─ for each command:
  │     │           │     ├─ material->Bind(cmd)  // BindDescriptorSet(0, ...)
  │     │           │     ├─ PushConstants(MVP, Color)
  │     │           │     ├─ SetVertexBuffer / SetIndexBuffer
  │     │           │     └─ DrawIndexed(6)
  │     ├─ Renderer::ClearQueue()
  │                                │
  └─ RenderSystem::Present()  ─────┘
        └─ device->Present()
              └─ vkQueuePresentKHR
```
