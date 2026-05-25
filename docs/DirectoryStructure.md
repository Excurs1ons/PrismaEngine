# Directory Structure / 目录结构

This document describes the directory organization of Prisma Engine.

本文档描述 Prisma Engine 的目录组织结构。

## Root Structure / 根目录结构

```
PrismaEngine/
├── src/                      # Source code / 源代码
├── resources/                # Engine resources / 引擎资源
├── projects/                 # Sample projects / 示例项目
├── sdk/                      # Public SDK headers + CMake config
├── cmake/                    # CMake modules / CMake 模块
├── docs/                     # Documentation / 文档
├── assets/                   # Shaders and demo assets / 着色器和示例资产
├── .dependencies/            # Vendored third-party libraries / 第三方库缓存
├── build*/                   # Build outputs (gitignored) / 构建输出
└── scripts/                  # Build and setup scripts / 构建和设置脚本
```

## Source Code / src/

### Core Engine / src/engine/

```
src/engine/
├── app/                     # Engine application layer
│   ├── Engine.h/cpp         # Main engine lifecycle
│   ├── Application.*        # Application interface
│   ├── ECSApplication.*     # ECS-based application
│   ├── EditorApplication.*  # Editor-mode application
│   ├── EngineCAPI.*         # C API for engine interop
│   └── ProjectConfig.h      # Project configuration
│
├── audio/                    # Audio system / 音频系统
│   ├── AudioAPI.h/cpp        # Audio API interface
│   ├── AudioDevice.{h,cpp}   # Audio device abstraction
│   ├── AudioDeviceSDL3.*     # SDL3 audio backend
│   ├── backends/             # Audio backend implementations
│   │   └── AudioDeviceMiniaudio.*  # Miniaudio backend
│   ├── codecs/               # Audio codec decoders
│   │   ├── WavDecoder.h
│   │   ├── FlacDecoder.h
│   │   ├── Mp3Decoder.h
│   │   └── OggDecoder.h
│   ├── components/           # ECS audio components
│   │   ├── AudioSourceComponent.h
│   │   ├── AudioListenerComponent.h
│   │   ├── ReverbZoneComponent.h
│   │   └── AudioSystem.h
│   ├── dsp/                  # DSP node graph engine
│   │   ├── AudioNode.h       # Base DSP node
│   │   ├── AudioBuffer.h     # Audio buffer management
│   │   └── nodes/            # 22 DSP node types
│   │       ├── OscillatorNode.h / DelayNode.h / ReverbNode.h
│   │       ├── BiquadFilterNode.h / SVFNode.h / CompressorNode.h
│   │       ├── ConvolutionReverbNode.h / ChorusNode.h / FlangerNode.h
│   │       └── ... (22 total DSP node types)
│   └── raytracing/           # Acoustic raytracing
│       ├── AcousticEngine.h
│       └── AcousticRay.h
│
├── config/                   # Engine configuration
│   ├── EngineConfig.h
│   ├── RenderBackendConfig.h
│   └── AudioBackendConfig.h
│
├── core/                     # Core engine components / 核心引擎组件
│   ├── ECS.h/cpp             # Entity Component System
│   ├── Component.h/cpp       # Component base
│   ├── ComponentRegistry.*   # Component type registry
│   ├── EntityManager.*       # Entity management
│   ├── AssetManager.h/cpp    # Asset loading and management
│   ├── Asset.h               # Base asset interface
│   ├── Handle.h              # Type-safe Handle<T> generational index
│   ├── Event.h               # Event system
│   ├── Layer.h/cpp           # Application Layer system
│   ├── Node.h/cpp            # Scene Node
│   ├── UUID.h/cpp            # UUID generation
│   ├── Singleton.h           # Singleton base
│   └── Systems.h             # System definitions
│
├── graphic/                  # Rendering system / 渲染系统
│   ├── adapters/vulkan/      # Vulkan RHI implementation
│   │   ├── RenderDeviceVulkan.*    # Main Vulkan device
│   │   ├── VulkanCommandBuffer.*   # Command buffer wrapper
│   │   ├── VulkanResources.*       # Resource management
│   │   ├── VulkanShader.*          # Shader compilation
│   │   ├── VulkanSwapChain.*       # Swap chain
│   │   └── ... (16+ Vulkan adapter files)
│   │
│   ├── interfaces/           # Rendering RHI interfaces / 渲染接口
│   │   ├── IRenderDevice.h   # Device context interface
│   │   ├── ICommandBuffer.h  # Command recording interface
│   │   ├── IBuffer.h         # Buffer interface
│   │   ├── ITexture.h        # Texture interface
│   │   ├── IShader.h         # Shader interface
│   │   ├── IPipeline.h       # Pipeline interface
│   │   ├── IResourceFactory.h
│   │   └── ... (20+ RHI interface files)
│   │
│   ├── pipelines/            # Render pipelines / 渲染管线
│   │   ├── forward/          # Forward rendering
│   │   │   ├── ForwardPipeline.*
│   │   │   ├── OpaquePass.*
│   │   │   ├── TransparentPass.*
│   │   │   └── DepthPrePass.*
│   │   ├── deferred/         # Deferred rendering
│   │   │   ├── DeferredPipeline.*
│   │   │   ├── GBuffer.* / GeometryPass.*
│   │   │   ├── LightingPass.* / CompositionPass.*
│   │   ├── clustered/        # Clustered forward rendering
│   │   │   ├── ClusteredForwardPipeline.*
│   │   │   └── ClusteredOpaquePass.*
│   │   ├── npr/              # Non-photorealistic rendering
│   │   │   ├── NPRPipeline.*
│   │   │   └── NPROpaquePass.*
│   │   └── pathtracing/      # Path tracing (Vulkan RT + software)
│   │       ├── PathTracingPipeline.*
│   │       └── VulkanRTBackend.*
│   │
│   ├── 2d/                   # 2D rendering system
│   │   ├── Pipeline2D.*      # 2D render pipeline
│   │   ├── Graphics2D.*      # 2D graphics API
│   │   ├── CanvasPass2D.*    # Canvas rendering pass
│   │   ├── Light2D.*         # 2D lighting
│   │   ├── LightManager2D.*  # 2D light management
│   │   ├── PostProcessPass2D.*
│   │   └── UIPass2D.*
│   │
│   ├── ui/                   # UI rendering components
│   │   ├── FontAtlas.*       # Font atlas generation
│   │   ├── TextRendererComponent.*
│   │   └── UIPass.*
│   │
│   ├── RenderGraph.*         # Render graph system
│   ├── Renderer.*            # Main renderer
│   ├── Mesh.h/cpp            # Mesh geometry
│   ├── Material.h/cpp        # Material system
│   ├── Shader.h/cpp          # Shader abstraction
│   ├── TextureAtlas.*        # Texture atlas
│   ├── VoxelRenderer.*       # Voxel/chunk rendering
│   └── ... (80+ rendering files total)
│
├── input/                    # Input system / 输入系统
│   ├── InputManager.*        # Input management
│   ├── InputDevice.*         # Device abstraction
│   ├── EnhancedInputManager.h # Enhanced input with action mappings
│   ├── core/IInputDriver.h   # Input driver interface
│   └── drivers/              # Platform input drivers
│       ├── InputDriverSDL3.*
│       └── InputDriverWin32.*
│
├── math/                     # Mathematics library / 数学库
│   ├── MathTypes.h           # Unified math types (Vector3, Matrix4, etc.)
│   └── MatrixUtils.h         # Matrix helper functions
│
├── physics/                  # Physics system
│   ├── CollisionSystem.h     # AABB collision, raycast, sweep
│   ├── PhysicsSystem.*       # Physics simulation
│   └── RigidBody.h
│
├── platform/                 # Platform abstraction / 平台抽象层
│   ├── Platform.h/cpp        # Platform detection and utilities
│   ├── DynamicLoader.*       # Dynamic library loading
│   └── Platform.cpp          # Platform implementation
│
├── resource/                 # Resource management / 资源管理
│   ├── AssetSerializer.*     # Asset serialization
│   ├── Archive.*             # Archive formats (JSON, binary)
│   ├── TextureAsset.*        # Texture loading
│   ├── MeshAsset.*           # Mesh loading
│   ├── OBJParser.*           # OBJ file parser
│   ├── CubemapTextureAsset.* # Cubemap loading
│   ├── ResourceFallback.*    # Fallback resources
│   └── embedded/             # Embedded resources
│
├── scripting/                # Scripting system / 脚本系统
│   ├── CoreCLRHost.*         # .NET CoreCLR self-hosted runtime
│   ├── ScriptEngine.*        # Script engine manager
│   ├── ScriptSystem.*        # Script lifecycle system
│   ├── SRPGraphicsAPI.*      # Scriptable Render Pipeline C++ API
│   ├── EditorAPI.*           # Editor scripting API
│   └── CSharp/               # C# source files
│       ├── Prisma.Core/      # PrismaEngine.Core managed library
│       │   ├── Script.cs / Node.cs / World.cs
│       │   ├── SRP/           # C# SRP bindings (CommandBuffer, Pipeline, etc.)
│       │   ├── DSPNodes/     # C# DSP node wrappers
│       │   ├── UI/           # C# UI components (Button, Canvas, Text, etc.)
│       │   └── ... (80+ C# files)
│       ├── Prisma.Generators/ # Roslyn source generators
│       └── PrismaEngine.Host/ # Self-contained .NET host
│
├── serialization/            # Serialization
│   ├── Serializable.*
│   └── ScriptComponent.*
│
├── threading/                # Threading / 线程系统
│   ├── JobSystem.*           # Job-based task system
│   ├── ThreadManager.*       # Thread pool manager
│   └── WorkerThread.*        # Worker thread abstraction
│
├── transform/                # Transform system
│   ├── Transform.*           # Transform component
│   ├── Camera.*              # Camera component
│   └── CameraController.*    # Camera control logic
│
├── ui/                       # UI system
│   ├── 2d/                   # 2D UI components
│   │   ├── CanvasComponent.*
│   │   └── ButtonComponent.*
│   ├── UIComponent.*
│   └── UIInputManager.*
│
├── window/                   # Window management
│   └── Window.*
│
├── logger/                   # Logger system / 日志系统
│   ├── Logger.*
│   └── LogScope.*
│
├── utils/                    # Utilities
│   └── ImageUtils.*
│
├── object/                   # Object model
│   ├── Object.*
│   └── Model.h
│
└── scene/                    # Scene management
    ├── Scene.*
    ├── SceneManager.*
    └── SceneNode.*
```

### Launcher / src/launcher/

Platform-specific runtime entry point.

平台特定的运行时入口点。

```
src/launcher/
└── core/
    └── LauncherMain.cpp     # Platform-independent launcher entry
```

### Editor / src/editor/

```
src/editor/
├── core/                    # Editor core
│   ├── Editor.*             # Main editor application
│   ├── EditorService.*      # Transport-agnostic editor backend
│   ├── WebUIEditor.*        # Browser-based editor (HTTP server)
│   ├── CommandLineEditor.*
│   └── Environment.*
│
├── mcp/                     # Model Context Protocol server
│   ├── MCPServer.*          # JSON-RPC router
│   ├── MCPSession.*         # Session management + hash delta
│   ├── MCPSubSystem.*       # Engine lifecycle integration
│   ├── MCPTool.*            # Tool registry
│   ├── serialization/       # MCP JSON serialization
│   ├── transport/           # Transport layer (Stdio, TCP)
│   ├── session/             # Delta tracker, Token budget
│   └── tools/               # 17 tools (Scene, ECS, Engine, Debug, Asset, Editor, Game)
│
├── panels/                  # ImGui editor panels
│   ├── EditorLayer.*
│   └── ProfilerPanel.*
│
├── windows/                 # Editor windows
│   └── ProjectSettingsWindow.*
│
└── graphic/                 # Editor graphics
    ├── ImGuiVulkanResourceManager.*
    └── ViewportRenderPass.*
```

### Game / src/game/

```
src/game/
└── Game.h                   # Game framework interface
```

## Resources / resources/

Platform-specific resources.

平台特定资源。

```
resources/
└── windows/                 # Windows platform resources
    └── icons/               # Windows icons
        ├── Launcher.ico     # Main application icon
        └── small.ico        # Small icon
```

> **Note**: Shader source files (HLSL/GLSL) are located in the `assets/` directory.
> 
> **说明**：着色器源码（HLSL/GLSL）位于 `assets/` 目录下。

## Platform Projects / projects/

### Project List

```
projects/
├── PacManGame/              # 2D 吃豆人游戏示例
├── PrismaCraft/             # Minecraft 风格体素游戏
├── Prisma2D/                # 2D 游戏项目模板（含 C# 脚本）
├── PathTracing3D/           # 3D 路径追踪模板（含 Hardware RT + SSBO）
├── ClusteredForward3D/      # 聚簇前向渲染演示
├── Deferred3D/              # 延迟渲染演示（含 SSGI）
├── SRP2D/                   # 2D Scriptable Render Pipeline 示例
└── NeoEditor/               # WinUI3 C# 编辑器原型
```

> **Note**: The Android project (`projects/android/PrismaAndroid/`) has been migrated into the engine's core build system.
>
> **说明**：Android 项目（`projects/android/PrismaAndroid/`）已整合到引擎核心构建系统中。

## Assets / assets/

Example/demo game assets (not part of the engine).

示例/演示游戏资产（不是引擎的一部分）。

```
assets/
└── shaders/                 # Shader source files
    ├── pbr_*.glsl / pbr_*.vert / pbr_*.frag        # PBR shaders
    ├── npr_*.glsl / npr_*.vert / npr_*.frag        # NPR toon shaders
    ├── clustered/                                    # Clustered forward shaders
    ├── LitSprite.frag / UnlitSprite.frag             # 2D sprite shaders
    ├── PointLight2D.* / ReflectionSprite.frag        # 2D light shaders
    └── pbr_ibl_*.comp                                # IBL compute shaders
```

## CMake Modules / cmake/

```
cmake/
├── CompilerOptions.cmake        # Compiler flags and warnings
├── DependencyVersions.cmake     # Version lock for all dependencies
├── DeviceOptions.cmake          # Device/platform-specific options
├── EditorPostTargets.cmake      # Editor post-build steps
├── EditorTargets.cmake          # Editor build targets
├── EngineTargets.cmake          # Engine library targets
├── FetchThirdPartyDeps.cmake    # FetchContent dependency management (15+ deps)
├── InstallConfig.cmake          # Install configuration
├── LauncherTargets.cmake        # Launcher build targets
├── OutputDirectories.cmake      # Output directory configuration
├── PackagingConfig.cmake        # Packaging configuration
├── PlatformConfig.cmake         # Platform-specific config
├── ProjectTargets.cmake         # Project build targets
├── SDKConfig.cmake              # SDK generation config
└── Utils.cmake                  # Utility functions
```

## Documentation / docs/

```
docs/
├── Index.md                  # Documentation index (start here)
├── Roadmap.md                # Module status, code stats, future plans
├── Architecture.md           # High-level system design
├── RenderingSystem.md        # Rendering system docs
├── VulkanIntegration.md      # Android/Vulkan integration
├── AssetSerialization.md     # Asset serialization
├── AudioSystem.md            # Audio system architecture
├── ScriptingSystem.md        # CoreCLR scripting docs
├── DirectoryStructure.md     # This file
├── ...                       # See Index.md for full list (50+ doc files)
├── plans/                    # Implementation plans (date-prefixed)
└── plans/archived/           # Completed/archived plans
```

> **Note**: The root-level `CLAUDE.md`, `README.md`, and `GEMINI.md` are project-level configuration files, not documentation.
>
> **说明**: 根目录下的 `CLAUDE.md`、`README.md`、`GEMINI.md` 是项目级配置文件，而非文档。

## Build Artifacts (Gitignored) / 构建产物（Git忽略）

```
build*/                       # CMake build directories
*.vcxproj.user               # Visual Studio user files
.vs/                         # Visual Studio configuration
projects/android/PrismaAndroid/app/.cxx/  # Android native build
projects/android/PrismaAndroid/.gradle/   # Gradle cache
```

## File Naming Conventions / 文件命名约定

| Type / 类型 | Convention / 约定 | Example / 示例 |
|-------------|-----------------|----------------|
| Headers / 头文件 | PascalCase.h | `AssetManager.h` |
| Source / 源文件 | PascalCase.cpp | `AssetManager.cpp` |
| Shaders (HLSL) | PascalCase.hlsl | `Skybox.hlsl` |
| Shaders (GLSL) | lowercase.vert/frag | `skybox.vert` |
| Assets / 资产 | PascalCase.ext | `PlayerModel.fbx` |
| Config / 配置 | lowercase.json | `settings.json` |

## Namespace Organization / 命名空间组织

```cpp
namespace PrismaEngine {
    namespace Graphic {        // Rendering system / 渲染系统
    namespace Audio {          // Audio system / 音频系统
    namespace Input {          // Input system / 输入系统
    namespace Resource {       // Resource management / 资源管理
    namespace Platform {       // Platform abstraction / 平台抽象
    namespace Scripting {      // Scripting / 脚本
}
```

## Adding New Code / 添加新代码

### Adding a New System / 添加新系统
1. Create directory in `src/engine/` / 在 `src/engine/` 中创建目录
2. Create headers and source files / 创建头文件和源文件
3. Add to `src/engine/CMakeLists.txt` / 添加到 CMakeLists.txt
4. Use `PrismaEngine` namespace / 使用 `PrismaEngine` 命名空间

### Adding Platform-Specific Code / 添加平台特定代码
1. Create implementation in `src/launcher/{platform}/` / 在启动器目录创建实现
2. Use platform abstraction in `src/engine/platform/` / 使用平台抽象层
3. Add conditional compilation if needed / 如需要使用条件编译

### Adding Resources / 添加资源
- **Shaders**: `assets/shaders/` (HLSL or GLSL)
- **Platform-specific resources**: `resources/{platform}/`

## Related Documentation / 相关文档
- [Documentation Index](Index.md) - Full documentation map / 完整文档索引
- [Rendering System](RenderingSystem.md) - Rendering architecture / 渲染架构
- [Vulkan Integration](VulkanIntegration.md) - Android/Vulkan setup
