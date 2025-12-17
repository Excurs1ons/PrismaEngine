# PrismaEngine 渲染系统重设计

## 概述

本文档描述了对 PrismaEngine 渲染系统的重设计方案，旨在解决当前架构中职责不分明、组件间耦合过紧的问题。

## 当前架构问题

### 1. 职责冲突
- **RenderSystem** 与 **RenderBackend** 都控制帧流程
- 资源所有权分散：Backend创建、System分发、Pipeline使用但不拥有
- Backend包含了Pipeline特定的逻辑（如默认PSO创建）

### 2. 紧耦合
- System直接操作Backend特定状态
- Pipeline直接依赖Backend实现
- 初始化链式依赖，无法优雅降级

### 3. 抽象层级混乱
- 低级图形API操作与高级渲染逻辑混合
- 缺乏清晰的抽象边界

## 新架构设计

### 核心原则

1. **单一职责原则**：每个组件只负责一个明确的功能
2. **依赖倒置**：高层模块不依赖低层模块，都依赖抽象
3. **开闭原则**：对扩展开放，对修改关闭
4. **关注点分离**：不同抽象层级严格分离

### 架构分层

```
┌─────────────────────────────────────┐
│         Application Layer            │  ← Game/Editor
├─────────────────────────────────────┤
│        Render Interface             │  ← IRenderDevice, ICommandBuffer
├─────────────────────────────────────┤
│       Pipeline System               │  ← RenderPipeline, RenderPass
├─────────────────────────────────────┤
│       Resource Manager              │  ← Texture, Buffer, Shader管理
├─────────────────────────────────────┤
│      Device Abstraction             │  ← RenderDevice, ResourceFactory
├─────────────────────────────────────┤
│       Platform Backend              │  → DirectX12, Vulkan, Metal
└─────────────────────────────────────┘
```

## 详细组件设计

### 1. RenderSystem (渲染系统管理器)

**职责**：
- 管理渲染系统的生命周期
- 选择和初始化渲染后端
- 协调各子系统工作
- 提供统一的渲染接口

**不负责**：
- 具体的渲染实现
- 资源的生命周期管理
- 渲染管道的执行细节

```cpp
class RenderSystem : public ManagerBase<RenderSystem> {
public:
    // 初始化系统，选择后端
    bool Initialize(const RenderSystemDesc& desc);

    // 帧控制
    void BeginFrame();
    void EndFrame();
    void Present();

    // 获取抽象接口
    IRenderDevice* GetDevice() const { return m_device.get(); }
    IResourceManager* GetResourceManager() const { return m_resourceManager.get(); }

private:
    std::unique_ptr<IRenderDevice> m_device;
    std::unique_ptr<IResourceManager> m_resourceManager;
    std::unique_ptr<RenderPipelineRegistry> m_pipelineRegistry;
};
```

### 2. IRenderDevice (设备抽象接口)

**职责**：
- 提供设备级别的抽象
- 创建和管理命令缓冲区
- 资源创建工厂
- 同步操作

**不负责**：
- 具体的渲染逻辑
- 资源的生命周期（由ResourceManager负责）
- 渲染管道的组织

```cpp
class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    // 命令缓冲区管理
    virtual std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) = 0;
    virtual void SubmitCommandBuffer(ICommandBuffer* cmdBuffer) = 0;

    // 同步
    virtual void WaitForIdle() = 0;
    virtual std::unique_ptr<IFence> CreateFence() = 0;

    // 资源工厂
    virtual IResourceFactory* GetResourceFactory() const = 0;

    // 交换链
    virtual ISwapChain* GetSwapChain() const = 0;
};
```

### 3. ICommandBuffer (命令缓冲区)

**职责**：
- 记录渲染命令
- 提供后端无关的命令接口
- 支持命令批量提交

```cpp
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    // 基础命令
    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual void BeginRenderPass(IRenderPass* renderPass) = 0;
    virtual void EndRenderPass() = 0;

    // 渲染命令
    virtual void SetPipeline(IPipeline* pipeline) = 0;
    virtual void SetVertexBuffer(IBuffer* buffer, uint32_t slot = 0) = 0;
    virtual void SetIndexBuffer(IBuffer* buffer) = 0;
    virtual void SetConstantBuffer(IBuffer* buffer, uint32_t slot) = 0;
    virtual void SetTexture(ITexture* texture, uint32_t slot) = 0;

    virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0) = 0;
};
```

### 4. RenderPipeline (渲染管道)

**职责**：
- 管理RenderPass序列
- 控制渲染流程
- 管理管道特定资源

```cpp
class RenderPipeline {
public:
    virtual ~RenderPipeline() = default;

    // 渲染流程控制
    virtual void Execute(IRenderContext* context, const RenderPipelineDesc& desc) = 0;
    virtual void AddRenderPass(std::unique_ptr<RenderPass> pass) = 0;
    virtual void RemoveRenderPass(const std::string& name) = 0;

    // 资源管理
    virtual IRenderPass* GetRenderPass(const std::string& name) = 0;

protected:
    std::vector<std::unique_ptr<RenderPass>> m_renderPasses;
};
```

### 5. IResourceManager (资源管理器)

**职责**：
- 集中管理所有渲染资源
- 处理资源加载和卸载
- 提供资源池化机制
- 跟踪资源依赖

```cpp
class IResourceManager {
public:
    virtual ~IResourceManager() = default;

    // 资源加载
    virtual std::shared_ptr<ITexture> LoadTexture(const TextureDesc& desc) = 0;
    virtual std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;
    virtual std::shared_ptr<IShader> LoadShader(const ShaderDesc& desc) = 0;
    virtual std::shared_ptr<IPipeline> CreatePipeline(const PipelineDesc& desc) = 0;

    // 资源查询
    virtual std::shared_ptr<IResource> GetResource(ResourceId id) = 0;
    virtual void ReleaseResource(ResourceId id) = 0;

    // 垃圾回收
    virtual void GarbageCollect() = 0;
};
```

### 6. ResourceFactory (资源工厂)

**职责**：
- 创建特定后端的资源对象
- 封装平台特定的实现细节

```cpp
class IResourceFactory {
public:
    virtual ~IResourceFactory() = default;

    // 后端特定的资源创建
    virtual std::unique_ptr<ITexture> CreateTextureImpl(const TextureDesc& desc) = 0;
    virtual std::unique_ptr<IBuffer> CreateBufferImpl(const BufferDesc& desc) = 0;
    virtual std::unique_ptr<IShader> CreateShaderImpl(const ShaderDesc& desc) = 0;
    virtual std::unique_ptr<IPipeline> CreatePipelineImpl(const PipelineDesc& desc) = 0;
};
```

## 渲染流程重设计

### 帧渲染流程

```
1. Application (Game Loop)
   ↓
2. RenderSystem::BeginFrame()
   ↓
3. RenderPipeline::Execute()
   ├─ RenderPass 1 (Shadow Pass)
   │  ├─ CommandBuffer::Begin()
   │  ├─ SetPipeline/SetResources
   │  └─ Draw Commands
   ├─ RenderPass 2 (Geometry Pass)
   │  └─ ...
   └─ RenderPass N (Post Process)
      └─ ...
   ↓
4. RenderSystem::EndFrame()
   ↓
5. RenderSystem::Present()
```

### 初始化流程

```
1. Application creates RenderSystem
   ↓
2. RenderSystem selects and creates IRenderDevice implementation
   ├─ DirectX12Device
   └─ VulkanDevice (future)
   ↓
3. RenderDevice creates ResourceFactory
   ↓
4. RenderSystem creates ResourceManager
   ↓
5. Application creates and registers RenderPipelines
   ↓
6. Start rendering loop
```

## 数据流设计

### 资源生命周期

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│ Application │───→ │ ResourceManager │───→ │  ResourcePool │
└─────────────┘     └─────────────┘     └─────────────┘
       │                    │                    │
       └─── Weak References ─────────────────────┘
```

### 命令流程

```
RenderPass
   │ (Creates and records)
   ↓
ICommandBuffer
   │ (Submits to)
   ↓
IRenderDevice
   │ (Executes on)
   ↓
GPU Backend (DX12/Vulkan)
```

## 迁移计划

### 阶段1：基础抽象层
1. 定义接口类 (IRenderDevice, ICommandBuffer, IResourceManager等)
2. 创建工厂类和基础实现框架
3. 保留现有实现作为向后兼容

### 阶段2：资源管理重构
1. 实现ResourceManager和ResourceFactory
2. 迁移现有资源管理代码
3. 实现资源池化和生命周期管理

### 阶段3：渲染管道重构
1. 重构ScriptableRenderPipeline使用新的抽象
2. 修改RenderPass使用ICommandBuffer
3. 分离渲染逻辑与后端实现

### 阶段4：后端重构
1. 重构DirectX12实现使用新接口
2. 完成Vulkan后端实现
3. 移除旧的耦合代码

### 阶段5：清理和优化
1. 移除向后兼容代码
2. 性能优化和测试
3. 文档更新

## 预期收益

1. **清晰的责任划分**：每个组件职责单一明确
2. **易于扩展**：新后端和渲染特性更容易添加
3. **更好的可测试性**：各层可以独立测试
4. **性能优化空间**：资源管理和命令提交可以更优化
5. **维护性提升**：代码结构更清晰，维护更容易

## 风险和缓解

### 风险
1. 重构工作量较大
2. 可能引入临时性能下降
3. 需要大量测试

### 缓解措施
1. 分阶段进行，保持系统可用
2. 保留关键性能基准
3. 充分的单元测试和集成测试