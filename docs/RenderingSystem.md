# PrismaEngine 渲染系统架构文档

## 概述

PrismaEngine的渲染系统采用现代化的模块化设计，支持多个图形API后端，并正在向RenderGraph架构迁移。当前实现包括ScriptableRenderPipeline系统，支持灵活的渲染通道组合。

## 架构概览

```
RenderSystem
├── RenderBackend (抽象层)
│   ├── RenderBackendDirectX12 (Windows)
│   ├── RenderBackendVulkan (Cross-platform)
│   └── RenderBackendSDL3 (Legacy)
├── ScriptableRenderPipeline
│   ├── ForwardRenderPass
│   ├── SkyboxRenderPass
│   └── GeometryRenderPass
└── RenderGraph (正在实现中)
    ├── ResourceHandle
    ├── RenderPassBuilder
    └── 自动依赖管理
```

## 核心组件

### 1. RenderSystem

渲染系统的主管理类，负责：
- 初始化和管理渲染后端
- 执行渲染管线
- 管理渲染回调（GUI等）

```cpp
class RenderSystem : public ManagerBase<RenderSystem> {
public:
    // 初始化渲染系统
    bool Initialize(Platform* platform, RenderBackendType type,
                   WindowHandle window, void* surface,
                   uint32_t width, uint32_t height);

    // 渲染流程控制
    void BeginFrame();
    void EndFrame();
    void Present();
    void Resize(uint32_t width, uint32_t height);

    // 获取渲染后端和管线
    RenderBackend* GetRenderBackend() const;
    ScriptableRenderPipeline* GetRenderPipeline() const;
};
```

### 2. RenderBackend (渲染后端)

抽象了不同图形API的差异：

#### DirectX 12 后端
- 支持Windows平台
- 原生支持Enhanced Barriers
- PIX调试工具集成

#### Vulkan 后端
- 跨平台支持（Windows、Android、Linux）
- 细粒度同步控制
- RenderDoc调试支持

### 3. ScriptableRenderPipeline

可编程渲染管线，支持动态添加渲染通道：

```cpp
class ScriptableRenderPipeline {
public:
    // 添加渲染通道
    void AddRenderPass(std::shared_ptr<RenderPass> renderPass);

    // 执行所有通道
    void Execute();

    // 设置视口大小
    void SetViewportSize(uint32_t width, uint32_t height);
};
```

### 4. RenderPass (渲染通道)

基类定义了渲染通道的接口：

```cpp
class RenderPass {
public:
    // 执行渲染
    virtual void Execute(RenderCommandContext* context) = 0;

    // 设置渲染目标
    virtual void SetRenderTarget(void* renderTarget) = 0;

    // 清屏操作
    virtual void ClearRenderTarget(float r, float g, float b, float a) = 0;

    // 设置视口
    virtual void SetViewport(uint32_t width, uint32_t height) = 0;
};
```

### 5. 具体渲染通道实现

#### ForwardRenderPass
前向渲染通道，支持：
- PBR材质渲染
- 多光源支持
- 阴影映射

#### SkyboxRenderPass
天空盒渲染：
- 立方体贴图采样
- 视图矩阵移除平移
- 深度缓冲优化

#### GeometryRenderPass
几何体渲染：
- 网格渲染
- 实例化渲染
- GPU蒙皮支持

## 使用示例

### 设置渲染管线

```cpp
// 创建渲染系统
auto renderSystem = RenderSystem::GetInstance();

// 初始化（自动选择后端）
renderSystem->Initialize(platform.get(),
                        RenderBackendType::Vulkan,
                        window, nullptr, 1920, 1080);

// 获取渲染管线
auto pipeline = renderSystem->GetRenderPipeline();

// 添加渲染通道
auto skyboxPass = std::make_shared<SkyboxRenderPass>();
auto forwardPass = std::make_shared<ForwardRenderPass>();

pipeline->AddRenderPass(skyboxPass);
pipeline->AddRenderPass(forwardPass);
```

### 创建自定义渲染通道

```cpp
class CustomRenderPass : public RenderPass {
public:
    void Execute(RenderCommandContext* context) override {
        // 设置着色器常量
        context->SetConstantBuffer("ViewProjection", viewProjectionMatrix);

        // 设置顶点和索引缓冲区
        context->SetVertexBuffer(vertexData, vertexSize, stride);
        context->SetIndexBuffer(indexData, indexSize, true);

        // 执行绘制
        context->DrawIndexed(indexCount);
    }

    // 实现其他必需的方法...
};
```

### 渲染循环

```cpp
// 游戏循环
while (running) {
    // 开始帧
    renderSystem->BeginFrame();

    // 场景渲染（通过管线自动执行）
    // pipeline->Execute() 在EndFrame前自动调用

    // GUI渲染（通过回调）
    if (guiCallback) {
        guiCallback(commandBuffer);
    }

    // 结束帧并呈现
    renderSystem->EndFrame();
    renderSystem->Present();
}
```

## RenderGraph 迁移

### 当前状态
正在从ScriptableRenderPipeline迁移到RenderGraph架构，详见：
[RenderGraph迁移计划](RenderGraph_Migration_Plan.md)

### 迁移优势
1. **自动资源管理**：智能的生命周期跟踪和内存别名
2. **依赖解析**：自动处理资源同步和屏障
3. **性能优化**：并行执行和批处理优化
4. **调试友好**：图形化的依赖关系查看器

### 迁移后的使用方式

```cpp
// 创建RenderGraph
RenderGraph graph;

// 创建资源
auto backbuffer = graph.getBackbuffer();
auto depthBuffer = graph.createTexture(
    ResourceDesc::DepthStencil(1920, 1080, Format::D32_Float),
    "DepthBuffer"
);

// 添加GBuffer通道
graph.addPass<GBufferData>("GBuffer")
    .write(colorTarget)
    .write(normalTarget)
    .write(depthBuffer)
    .setExecuteFunc([](RenderGraphContext& ctx, GBufferData& data) {
        renderGeometry(ctx);
    });

// 添加光照通道
graph.addPass<LightingData>("Lighting")
    .read(colorTarget)
    .read(normalTarget)
    .read(depthBuffer)
    .write(backbuffer)
    .setExecuteFunc([](RenderGraphContext& ctx, LightingData& data) {
        applyLighting(ctx);
    });

// 编译并执行
graph.compile();
graph.execute(renderBackend);
```

## 性能优化

### 已实现的优化
1. **多线程支持**：渲染命令准备并行化
2. **资源池化**：减少内存分配开销
3. **批处理**：减少Draw Call数量
4. **状态缓存**：避免重复的状态设置

### 计划中的优化（RenderGraph）
1. **自动资源别名**：内存重用
2. **屏障优化**：最小化GPU同步
3. **异步计算**：利用Compute Queue
4. **GPU驱动渲染**：Indirect Draw

## 调试和性能分析

### 可用工具
- **GPU调试器**：RenderDoc、PIX、Nsight
- **性能分析**：集成性能标记和统计
- **日志系统**：详细的渲染日志
- **可视化工具**：RenderGraph依赖查看器（计划中）

### 调试技巧
1. 使用`LOG_DEBUG`宏跟踪渲染流程
2. 检查GPU状态和资源内容
3. 使用帧调试器分析Draw Call
4. 监控GPU时间和内存使用

## 扩展指南

### 添加新的渲染后端
1. 继承`RenderBackend`类
2. 实现所有纯虚函数
3. 创建对应的`RenderCommandContext`
4. 在`RenderBackendType`中添加新类型

### 创建渲染效果
1. 实现新的`RenderPass`
2. 编写对应的着色器
3. 添加到渲染管线
4. 测试和优化性能

## 最佳实践

1. **资源管理**
   - 使用RAII管理GPU资源
   - 避免频繁的创建/销毁
   - 利用对象池和缓存

2. **性能优化**
   - 批量处理相似操作
   - 最小化状态切换
   - 使用异步执行

3. **错误处理**
   - 检查所有GPU操作结果
   - 使用断言和日志
   - 提供降级方案

4. **跨平台考虑**
   - 使用抽象层
   - 测试所有目标平台
   - 考虑硬件差异

## 未来计划

1. **完成RenderGraph迁移**（Q1 2024）
2. **实时光线追踪支持**（Q2 2024）
3. **GPU驱动渲染**（Q3 2024）
4. **AI加速渲染**（Q4 2024）

## 相关资源

- [DirectX 12 文档](https://docs.microsoft.com/en-us/windows/win32/directx12)
- [Vulkan 规范](https://www.khronos.org/vulkan/)
- [RenderGraph 设计模式](https://github.com/google/filament/blob/master/docs/Filament.md#rendergraph)