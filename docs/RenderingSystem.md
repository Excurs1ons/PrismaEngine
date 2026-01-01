# Rendering System Architecture / 渲染系统架构文档

## Overview / 概述

Prisma Engine's rendering system uses a modern modular design with support for multiple graphics API backends. The system is migrating towards a RenderGraph architecture while maintaining the current ScriptableRenderPipeline implementation.

PrismaEngine 的渲染系统采用现代化的模块化设计，支持多个图形API后端，并正在向 RenderGraph 架构迁移。当前实现包括 ScriptableRenderPipeline 系统，支持灵活的渲染通道组合。

## Architecture Overview / 架构概览

```
RenderSystem
├── RenderBackend (抽象层 / Abstraction)
│   ├── DirectX 12 (Windows) / DirectX 12 (Windows)
│   ├── Vulkan (Cross-platform) / Vulkan (跨平台)
│   └── [Future: Metal] (macOS/iOS)
│
├── Resource Adapters / 资源适配器
│   ├── DX12ResourceFactory (Windows)
│   └── VulkanResourceFactory (Cross-platform)
│
├── Render Pipelines / 渲染管线
│   ├── Forward Pipeline / 前向管线
│   │   ├── DepthPrePass
│   │   ├── OpaquePass
│   │   └── TransparentPass
│   └── Deferred Pipeline / 延迟管线
│       ├── GeometryPass
│       └── CompositionPass
│
└── RenderGraph (In Progress / 开发中)
    ├── ResourceHandle
    ├── PassBuilder
    └── Auto dependency management / 自动依赖管理
```

## Core Components / 核心组件

### 1. RenderSystem / 渲染系统

Main management class for the rendering system.

渲染系统的主管理类。

```cpp
namespace PrismaEngine {
namespace Graphic {

class RenderSystem {
public:
    // Initialize rendering system / 初始化渲染系统
    bool Initialize(Platform* platform, RenderAPI api,
                   WindowHandle window, void* surface,
                   uint32_t width, uint32_t height);

    // Frame control / 帧控制
    void BeginFrame();
    void EndFrame();
    void Present();
    void Resize(uint32_t width, uint32_t height);

    // Get rendering backend / 获取渲染后端
    IRenderBackend* GetBackend() const;
};

} // namespace Graphic
} // namespace PrismaEngine
```

### 2. Rendering Backends / 渲染后端

#### DirectX 12 (Windows) / DirectX 12 (Windows)

Located in `src/engine/graphic/adapters/dx12/`:

位置：`src/engine/graphic/adapters/dx12/`：

- **DX12ResourceFactory**: Resource creation
- **Enhanced Barriers**: Enhanced Barrier support
- **PIX Integration**: Debug tool integration

#### Vulkan (Cross-platform) / Vulkan（跨平台）

Located in `src/engine/graphic/adapters/vulkan/`:

位置：`src/engine/graphic/adapters/vulkan/`：

- **VulkanShader**: SPIR-V shader loading
- Cross-platform: Windows, Linux, Android
- **RenderDoc Support**: Debugging support

### 3. Render Pipelines / 渲染管线

#### Forward Pipeline / 前向管线

Located in `src/engine/graphic/pipelines/forward/`:

位置：`src/engine/graphic/pipelines/forward/`：

```
ForwardPipeline/
├── DepthPrePass.*          # Pre-pass depth rendering
├── OpaquePass.*            # Opaque geometry
├── TransparentPass.*       # Transparent geometry
└── ForwardRenderPassBase.* # Base class
```

#### Deferred Pipeline / 延迟管线

Located in `src/engine/graphic/pipelines/deferred/`:

位置：`src/engine/graphic/pipelines/deferred/`：

```
DeferredPipeline/
├── GeometryPass.*          # Geometry rendering
├── CompositionPass.*       # Lighting composition
└── DeferredPipeline.*      # Pipeline coordinator
```

### 4. Resource Management / 资源管理

#### RenderDesc / 渲染描述符

Located in `src/engine/graphic/RenderDesc.h`:

位置：`src/engine/graphic/RenderDesc.h`：

```cpp
namespace PrismaEngine {
namespace Graphic {

// Mesh description / 网格描述
struct MeshDesc {
    const Vertex* vertices;
    size_t vertexCount;
    const uint32_t* indices;
    size_t indexCount;
};

// Material description / 材质描述
struct MaterialDesc {
    glm::vec4 albedo;
    float metallic;
    float roughness;
    float ao;
    std::shared_ptr<TextureAsset> albedoMap;
    std::shared_ptr<TextureAsset> normalMap;
};

// Texture description / 纹理描述
struct TextureDesc {
    uint32_t width;
    uint32_t height;
    Format format;
    TextureUsage usage;
    MemoryType memory;
};

} // namespace Graphic
} // namespace PrismaEngine
```

#### IResourceFactory / 资源工厂接口

```cpp
namespace PrismaEngine {
namespace Graphic {

class IResourceFactory {
public:
    // Create buffer / 创建缓冲区
    virtual std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;

    // Create texture / 创建纹理
    virtual std::shared_ptr<ITexture> CreateTexture(const TextureDesc& desc) = 0;

    // Create shader / 创建着色器
    virtual std::shared_ptr<IShader> CreateShader(const ShaderDesc& desc) = 0;
};

} // namespace Graphic
} // namespace PrismaEngine
```

## Shaders / 着色器

### HLSL Shaders (DirectX 12) / HLSL 着色器

Located in `resources/common/shaders/hlsl/`:

位置：`resources/common/shaders/hlsl/`：

```
hlsl/
├── default.hlsl             # Default mesh shader / 默认网格着色器
├── Skybox.hlsl              # Skybox rendering / 天空盒渲染
└── Text.hlsl                # Text rendering / 文本渲染
```

### GLSL Shaders (Vulkan/OpenGL) / GLSL 着色器

Located in `resources/common/shaders/glsl/`:

位置：`resources/common/shaders/glsl/`：

```
glsl/
├── clearcolor.vert/frag     # Clear color pass / 清屏通道
├── shader.vert/frag         # Standard mesh / 标准网格
└── skybox.vert/frag         # Skybox / 天空盒
```

### Shader Compilation / 着色器编译

| Platform / 平台 | Compiler / 编译器 | Output / 输出 |
|-----------------|-------------------|--------------|
| Windows (DX12) | fxc/dxc | DXIL bytecode |
| Windows (Vulkan) | glslangValidator | SPIR-V |
| Android | Android Gradle Plugin | SPIR-V (automatic) |
| Linux | glslangValidator | SPIR-V |

## Platform-Specific Rendering / 平台特定渲染

### Windows Rendering / Windows 渲染

- **Primary API**: DirectX 12
- **Fallback**: Vulkan (optional)
- Entry point: `src/runtime/windows/WindowsRuntime.cpp`

### Linux Rendering / Linux 渲染

- **Primary API**: Vulkan
- Entry point: `src/runtime/linux/LinuxRuntime.cpp`

### Android Rendering / Android 渲染

- **Primary API**: Vulkan
- **Shaders**: Automatically compiled from GLSL to SPIR-V
- **Assets**: Loaded via `AAssetManager`
- Entry point: `src/runtime/android/AndroidRuntime.cpp`

See: [Android Integration](VulkanIntegration.md) for details.

详见：[Android 集成](VulkanIntegration.md)

## Camera System / 相机系统

### Camera Class / 相机类

Located in `src/engine/graphic/Camera.h`:

位置：`src/engine/graphic/Camera.h`：

```cpp
namespace PrismaEngine {
namespace Graphic {

class Camera {
public:
    // Get view matrix / 获取视图矩阵
    glm::mat4 GetViewMatrix() const;

    // Get projection matrix / 获取投影矩阵
    glm::mat4 GetProjectionMatrix() const;

    // Get view-projection matrix / 获取视图-投影矩阵
    glm::mat4 GetViewProjectionMatrix() const;

    // Camera controls / 相机控制
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetFieldOfView(float fov);
    void SetAspectRatio(float aspect);

private:
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    float m_fov;
    float m_aspect;
    float m_near;
    float m_far;
};

} // namespace Graphic
} // namespace PrismaEngine
```

## Material System / 材质系统

### Material Class / 材质类

Located in `src/engine/graphic/Material.h`:

位置：`src/engine/graphic/Material.h`：

```cpp
namespace PrismaEngine {
namespace Graphic {

class Material {
public:
    // Material properties / 材质属性
    void SetAlbedo(const glm::vec4& albedo);
    void SetMetallic(float metallic);
    void SetRoughness(float roughness);

    // Texture maps / 纹理贴图
    void SetAlbedoMap(std::shared_ptr<TextureAsset> texture);
    void SetNormalMap(std::shared_ptr<TextureAsset> texture);
    void SetRoughnessMap(std::shared_ptr<TextureAsset> texture);

    // Serialization / 序列化
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& data);
};

} // namespace Graphic
} // namespace PrismaEngine
```

## Usage Examples / 使用示例

### Setting Up Rendering / 设置渲染

```cpp
// Initialize render system / 初始化渲染系统
auto renderSystem = Graphic::RenderSystem::GetInstance();

renderSystem->Initialize(
    platform.get(),
    Graphic::RenderAPI::Vulkan,
    window, nullptr, 1920, 1080
);

// Create camera / 创建相机
auto camera = std::make_shared<Graphic::Camera>();
camera->SetPosition(glm::vec3(0.0f, 2.0f, 5.0f));

// Create material / 创建材质
auto material = std::make_shared<Graphic::Material>();
material->SetAlbedo(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
material->SetMetallic(0.5f);
material->SetRoughness(0.3f);
```

### Rendering Loop / 渲染循环

```cpp
// Game loop / 游戏循环
while (running) {
    // Begin frame / 开始帧
    renderSystem->BeginFrame();

    // Update camera / 更新相机
    camera->Update(deltaTime);

    // Render scene / 渲染场景
    renderSystem->RenderScene(scene, camera);

    // Render GUI / 渲染 GUI
    renderSystem->RenderGUI();

    // End frame and present / 结束帧并呈现
    renderSystem->EndFrame();
    renderSystem->Present();
}
```

## RenderGraph Migration / RenderGraph 迁移

### Current Status / 当前状态

Migrating from ScriptableRenderPipeline to RenderGraph architecture.

正在从 ScriptableRenderPipeline 迁移到 RenderGraph 架构。

See: [RenderGraph Migration Plan](RenderGraph_Migration_Plan.md)

### Migration Benefits / 迁移优势

1. **Automatic Resource Management** / 自动资源管理 - Smart lifecycle tracking
2. **Dependency Resolution** / 依赖解析 - Automatic synchronization
3. **Performance Optimization** / 性能优化 - Parallel execution
4. **Debug Friendly** / 调试友好 - Visual dependency viewer

### Future RenderGraph Usage / 未来的 RenderGraph 用法

```cpp
// Create RenderGraph / 创建 RenderGraph
RenderGraph graph;

// Create resources / 创建资源
auto backbuffer = graph.GetBackbuffer();
auto depthBuffer = graph.CreateTexture(
    ResourceDesc::DepthStencil(1920, 1080, Format::D32_Float),
    "DepthBuffer"
);

// Add GBuffer pass / 添加 GBuffer 通道
graph.AddPass<GBufferData>("GBuffer")
    .Write(colorTarget)
    .Write(normalTarget)
    .Write(depthBuffer)
    .SetExecuteFunc([](RenderGraphContext& ctx, GBufferData& data) {
        RenderGeometry(ctx);
    });

// Compile and execute / 编译并执行
graph.Compile();
graph.Execute(renderBackend);
```

## Performance Optimizations / 性能优化

### Implemented / 已实现

- **Multi-threading** / 多线程 - Parallel command preparation
- **Resource Pooling** / 资源池化 - Reduce allocation overhead
- **Batching** / 批处理 - Reduce draw calls
- **State Caching** / 状态缓存 - Avoid redundant state changes

### Planned (RenderGraph) / 计划中

- **Automatic Resource Aliasing** / 自动资源别名 - Memory reuse
- **Barrier Optimization** / 屏障优化 - Minimize GPU sync
- **Async Compute** / 异步计算 - Utilize compute queue
- **GPU-Driven Rendering** / GPU 驱动渲染 - Indirect draw

## Debugging and Profiling / 调试和性能分析

### Available Tools / 可用工具

- **GPU Debuggers**: RenderDoc, PIX, Nsight
- **Profiling**: Integrated performance markers
- **Logging**: Detailed rendering logs
- **Visual Tools**: RenderGraph dependency viewer (planned)

## Best Practices / 最佳实践

1. **Resource Management / 资源管理**
   - Use RAII for GPU resources / 使用 RAII 管理 GPU 资源
   - Avoid frequent creation/destruction / 避免频繁创建/销毁
   - Utilize object pools and caching / 利用对象池和缓存

2. **Performance / 性能**
   - Batch similar operations / 批量处理相似操作
   - Minimize state changes / 最小化状态切换
   - Use async execution / 使用异步执行

3. **Error Handling / 错误处理**
   - Check all GPU operation results / 检查所有 GPU 操作结果
   - Use assertions and logging / 使用断言和日志
   - Provide fallback options / 提供降级方案

4. **Cross-Platform / 跨平台**
   - Use abstraction layers / 使用抽象层
   - Test on all target platforms / 测试所有目标平台
   - Consider hardware differences / 考虑硬件差异

## Related Documentation / 相关文档
- [Directory Structure](DirectoryStructure.md) - File organization / 文件组织
- [Resource Management](ResourceManager.md) - Asset loading / 资产加载
- [Vulkan Integration](VulkanIntegration.md) - Android Vulkan setup / Android Vulkan 设置
- [Shader Compilation](#shader-compilation) - Shader workflows / 着色器工作流

## External Resources / 外部资源
- [DirectX 12 Documentation](https://docs.microsoft.com/en-us/windows/win32/directx12)
- [Vulkan Specification](https://www.khronos.org/vulkan/)
- [RenderGraph Design Pattern](https://github.com/google/filament/blob/master/docs/Filament.md#rendergraph)
