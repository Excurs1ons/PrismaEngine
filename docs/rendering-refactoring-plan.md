# 渲染系统重构实施计划

## 概述

本文档提供了将PrismaEngine渲染系统从当前架构重构到新架构的详细实施计划。

## 实施策略

### 分阶段重构
- 保持每个阶段后系统仍可运行
- 使用适配器模式处理新旧接口共存
- 优先保证核心功能不受影响
- 逐步迁移，最后移除旧代码

### 风险控制
- 每个阶段都有明确的回滚点
- 保留现有测试基准
- 并行开发新组件，减少阻塞

## 阶段1：基础抽象层定义 (预计2-3周)

### 任务1.1：定义核心接口 (3天)

**文件：** `src/engine/graphic/interfaces/IRenderDevice.h`
```cpp
// 定义IRenderDevice接口
class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;
    virtual bool Initialize(const DeviceDesc& desc) = 0;
    virtual void Shutdown() = 0;

    // 命令相关
    virtual std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) = 0;
    virtual void SubmitCommandBuffer(ICommandBuffer* cmdBuffer) = 0;

    // 资源工厂
    virtual IResourceFactory* GetResourceFactory() const = 0;

    // 同步
    virtual void WaitForIdle() = 0;
    virtual std::unique_ptr<IFence> CreateFence() = 0;
};
```

**文件：** `src/engine/graphic/interfaces/ICommandBuffer.h`
```cpp
// 定义ICommandBuffer接口
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;
    virtual void Begin() = 0;
    virtual void End() = 0;

    // 渲染命令
    virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;
    virtual void EndRenderPass() = 0;
    virtual void SetPipeline(IPipeline* pipeline) = 0;
    virtual void SetVertexBuffer(IBuffer* buffer, uint32_t slot) = 0;
    virtual void Draw(uint32_t vertexCount, uint32_t startVertex) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex) = 0;
};
```

**文件：** `src/engine/graphic/interfaces/IResourceManager.h`
```cpp
// 定义IResourceManager接口
class IResourceManager {
public:
    virtual ~IResourceManager() = default;

    // 资源创建和加载
    virtual std::shared_ptr<ITexture> LoadTexture(const std::string& path) = 0;
    virtual std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;
    virtual std::shared_ptr<IShader> LoadShader(const ShaderDesc& desc) = 0;

    // 资源管理
    virtual void ReleaseResource(ResourceId id) = 0;
    virtual void GarbageCollect() = 0;
};
```

### 任务1.2：定义资源接口 (2天)

**文件：**
- `src/engine/graphic/interfaces/ITexture.h`
- `src/engine/graphic/interfaces/IBuffer.h`
- `src/engine/graphic/interfaces/IShader.h`
- `src/engine/graphic/interfaces/IPipeline.h`

### 任务1.3：创建工厂接口 (1天)

**文件：** `src/engine/graphic/interfaces/IResourceFactory.h`

### 任务1.4：更新CMakeLists.txt (半天)
- 添加新的interfaces目录到构建系统

### 交付物
- 所有核心接口定义
- 接口文档
- 基础类型定义（ResourceId, CommandBufferType等）

## 阶段2：创建适配器层 (预计1周)

### 任务2.1：创建DirectX12适配器 (3天)

**文件：** `src/engine/graphic/backends/dx12/DX12RenderDevice.h`
```cpp
class DX12RenderDevice : public IRenderDevice {
private:
    // 保持对原有RenderBackendDirectX12的引用
    std::unique_ptr<RenderBackendDirectX12> m_legacyBackend;

public:
    DX12RenderDevice(RenderBackendDirectX12* backend);

    // 实现IRenderDevice接口
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) override {
        // 包装现有的命令列表创建逻辑
        return std::make_unique<DX12CommandBuffer>(m_legacyBackend.get());
    }
};
```

### 任务2.2：创建命令缓冲区适配器 (2天)

**文件：** `src/engine/graphic/backends/dx12/DX12CommandBuffer.h`
```cpp
class DX12CommandBuffer : public ICommandBuffer {
private:
    ID3D12GraphicsCommandList* m_commandList;
    RenderBackendDirectX12* m_backend;

public:
    DX12CommandBuffer(RenderBackendDirectX12* backend);

    // 转换调用到现有实现
    void Draw(uint32_t vertexCount, uint32_t startVertex) override {
        // 调用现有的Draw实现
    }
};
```

### 任务2.3：创建资源适配器 (2天)

- 为现有Buffer、Texture、Shader类创建包装器
- 实现新的接口但使用现有后端

### 交付物
- 完整的DirectX12适配器层
- 可以通过新接口使用现有功能

## 阶段3：重构RenderSystem (预计1周)

### 任务3.1：修改RenderSystem类 (2天)

**文件：** `src/engine/graphic/RenderSystem.h`
```cpp
class RenderSystem : public ManagerBase<RenderSystem> {
private:
    // 保持现有成员以确保兼容性
    std::unique_ptr<RenderBackendDirectX12> m_dx12Backend;  // 临时
    std::unique_ptr<IRenderDevice> m_device;  // 新接口

    // 新的组件
    std::unique_ptr<IResourceManager> m_resourceManager;

public:
    // 初始化方法改为使用新接口
    bool Initialize(const RenderSystemDesc& desc);

    // 获取新接口
    IRenderDevice* GetDevice() const { return m_device.get(); }
    IResourceManager* GetResourceManager() const { return m_resourceManager.get(); }

    // 保留现有接口的兼容性包装
    void RenderFrame();
};
```

### 任务3.2：创建ResourceManager实现 (3天)

**文件：** `src/engine/graphic/ResourceManager.h`
```cpp
class ResourceManager : public IResourceManager {
private:
    // 使用现有的资源管理系统
    std::unordered_map<ResourceId, std::shared_ptr<IResource>> m_resources;

public:
    ResourceManager(IRenderDevice* device);

    // 实现资源管理
    std::shared_ptr<ITexture> LoadTexture(const std::string& path) override {
        // 使用现有的资源加载逻辑
        // 返回适配器包装的对象
    }
};
```

### 任务3.3：更新初始化流程 (2天)

**修改：** `src/engine/EngineCore.cpp`
```cpp
// 修改渲染系统初始化
auto renderSystem = GetSystem<RenderSystem>();
RenderSystemDesc desc{};
desc.backendType = RenderBackendType::DirectX12;
desc.windowHandle = GetWindowHandle();
renderSystem->Initialize(desc);
```

### 交付物
- 使用新接口的RenderSystem
- 基础的ResourceManager实现
- 保持向后兼容的初始化流程

## 阶段4：重构渲染管道 (预计2周)

### 任务4.1：重构ScriptableRenderPipeline (3天)

**文件：** `src/engine/graphic/ScriptableRenderPipeline.h`
```cpp
class ScriptableRenderPipeline : public RenderPipeline {
private:
    IRenderDevice* m_device;
    IResourceManager* m_resourceManager;

public:
    ScriptableRenderPipeline(IRenderDevice* device, IResourceManager* resourceManager);

    void Execute(const RenderContext& context) override {
        // 创建命令缓冲区
        auto cmdBuffer = m_device->CreateCommandBuffer(CommandBufferType::Graphics);
        cmdBuffer->Begin();

        // 执行渲染通道
        for (auto& pass : m_renderPasses) {
            pass->Execute(cmdBuffer.get(), context);
        }

        cmdBuffer->End();
        m_device->SubmitCommandBuffer(cmdBuffer.get());
    }
};
```

### 任务4.2：更新RenderPass基类 (2天)

**文件：** `src/engine/graphic/RenderPass.h`
```cpp
class RenderPass {
public:
    virtual ~RenderPass() = default;

    // 使用新的命令缓冲区接口
    virtual void Execute(ICommandBuffer* cmdBuffer, const RenderContext& context) = 0;

protected:
    std::string m_name;
    RenderPassType m_type;
};
```

### 任务4.3：迁移现有RenderPass实现 (5天)

需要更新的文件：
- `src/engine/graphic/GeometryRenderPass.cpp`
- `src/engine/graphic/ForwardRenderPass.cpp`
- `src/engine/graphic/SkyboxRenderPass.cpp`
- `src/engine/graphic/RenderPass2D.cpp`

示例修改：
```cpp
void ForwardRenderPass::Execute(ICommandBuffer* cmdBuffer, const RenderContext& context) {
    // 使用新的命令缓冲区API
    cmdBuffer->BeginRenderPass(m_renderPassDesc);
    cmdBuffer->SetPipeline(m_pipeline.get());

    for (auto& drawable : context.drawables) {
        cmdBuffer->SetVertexBuffer(drawable.vertexBuffer, 0);
        cmdBuffer->SetIndexBuffer(drawable.indexBuffer);
        cmdBuffer->DrawIndexed(drawable.indexCount);
    }

    cmdBuffer->EndRenderPass();
}
```

### 任务4.4：更新TriangleExample (2天)

**文件：** `src/engine/TriangleExample.cpp`
- 使用新的渲染管道接口
- 确保示例程序正常运行

### 任务4.5：测试和调试 (2天)

- 运行所有测试
- 确保渲染结果正确
- 性能基准测试

### 交付物
- 使用新接口的渲染管道
- 更新的所有RenderPass实现
- 正常运行的TriangleExample

## 阶段5：完善资源管理 (预计1周)

### 任务5.1：实现资源池化 (3天)

**文件：** `src/engine/graphic/ResourcePool.h`
```cpp
template<typename T>
class ResourcePool {
private:
    std::queue<std::unique_ptr<T>> m_available;
    std::vector<std::unique_ptr<T>> m_all;

public:
    template<typename... Args>
    std::unique_ptr<T> Acquire(Args&&... args);

    void Release(std::unique_ptr<T> resource);
};
```

### 任务5.2：实现资源生命周期管理 (2天)

- 实现引用计数
- 添加延迟删除
- 处理资源依赖

### 任务5.3：优化资源加载 (2天)

- 实现异步加载
- 添加资源缓存
- 优化内存使用

### 交付物
- 完整的资源池化系统
- 优化的资源管理器
- 异步加载支持

## 阶段6：后端重构 (预计2周)

### 任务6.1：重构DirectX12后端 (5天)

**新文件：**
- `src/engine/graphic/backends/dx12/DX12Device.cpp`
- `src/engine/graphic/backends/dx12/DX12Buffer.cpp`
- `src/engine/graphic/backends/dx12/DX12Texture.cpp`
- `src/engine/graphic/backends/dx12/DX12Pipeline.cpp`

移除适配器，使用直接实现：
```cpp
class DX12Device : public IRenderDevice {
private:
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    // ...

public:
    // 直接实现接口，不再使用适配器
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) override {
        auto cmdList = CreateCommandList();
        return std::make_unique<DX12CommandBuffer>(cmdList);
    }
};
```

### 任务6.2：完善Vulkan后端 (5天)

**文件：** `src/engine/graphic/backends/vulkan/`
- 实现完整的VulkanDevice
- 完成资源管理
- 实现命令缓冲区

### 任务6.3：性能优化 (4天)

- 优化命令缓冲区提交
- 减少状态切换
- 批量处理资源更新

### 交付物
- 完全重构的DirectX12后端
- 功能完整的Vulkan后端
- 性能优化

## 阶段7：清理和文档 (预计1周)

### 任务7.1：移除旧代码 (2天)

删除的文件：
- `src/engine/graphic/RenderBackend.h` (如果不再需要)
- `src/engine/graphic/RenderBackendDirectX12.h` (如果完全迁移)
- 其他不再使用的文件

### 任务7.2：更新文档 (3天)

- 更新API文档
- 创建迁移指南
- 更新开发者指南

### 任务7.3：最终测试 (2天)

- 完整的回归测试
- 性能基准对比
- 内存泄漏检查

### 交付物
- 清理的代码库
- 完整的文档
- 测试报告

## 关键里程碑

| 里程碑 | 时间 | 交付物 | 成功标准 |
|--------|------|--------|----------|
| M1 | 第3周 | 基础接口定义 | 所有接口已定义并编译通过 |
| M2 | 第4周 | 适配器层 | 新接口可以调用现有功能 |
| M3 | 第5周 | RenderSystem重构 | 使用新接口但保持兼容性 |
| M4 | 第7周 | 渲染管道重构 | 所有RenderPass使用新接口 |
| M5 | 第8周 | 资源管理完善 | 资源池化和生命周期管理 |
| M6 | 第10周 | 后端重构完成 | DirectX12和Vulkan都使用新架构 |
| M7 | 第11周 | 清理完成 | 旧代码移除，文档完整 |

## 风险管理

### 高风险项
1. **性能回归**
   - 风险：新抽象层可能引入性能开销
   - 缓解：每个阶段都进行性能基准测试

2. **功能缺失**
   - 风险：重构可能遗漏某些功能
   - 缓解：保留现有测试，使用测试驱动开发

3. **时间超期**
   - 风险：重构工作量可能超出预期
   - 缓解：分阶段进行，可以调整优先级

### 应急计划
1. **回滚策略**：每个阶段都保留旧代码作为备份
2. **并行开发**：新功能可以在新架构上开发，不影响现有功能
3. **渐进迁移**：可以部分功能先使用新架构，逐步迁移

## 测试策略

### 单元测试
- 每个新接口都有对应的单元测试
- 使用Mock对象测试接口交互

### 集成测试
- TriangleExample作为基础集成测试
- 确保每个阶段后示例程序正常运行

### 性能测试
- Frame rate基准测试
- 内存使用监控
- CPU/GPU使用率分析

## 资源需求

### 人力资源
- 1名高级图形工程师（主导架构设计和核心实现）
- 1-2名工程师（协助实现和测试）

### 时间资源
- 总计约11周
- 可以根据团队大小调整并行度

### 工具和环境
- Visual Studio 2022
- DirectX 12 SDK
- Vulkan SDK
- 性能分析工具（PIX, RenderDoc等）

## 后续优化

重构完成后，可以进一步优化：
1. **多线程渲染**：利用新的命令缓冲区系统
2. **GPU驱动渲染**：实现更高级的GPU计算
3. **渲染优化**：实现更多渲染特性（延迟渲染、VR等）
4. **跨平台支持**：扩展支持更多平台（Linux, macOS等）

## 总结

本重构计划通过分阶段的方式，确保在改进架构的同时保持系统的稳定性和可用性。新架构将提供更好的抽象层次、更清晰的职责划分，以及更强的可扩展性，为PrismaEngine的未来发展奠定坚实基础。