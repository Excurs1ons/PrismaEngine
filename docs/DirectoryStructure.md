# Directory Structure / 目录结构

This document describes the directory organization of Prisma Engine.

本文档描述 Prisma Engine 的目录组织结构。

## Root Structure / 根目录结构

```
PrismaEngine/
├── src/                      # Source code / 源代码
├── resources/                # Engine resources / 引擎资源
├── projects/                 # Platform-specific projects / 平台特定项目
├── cmake/                    # CMake modules / CMake 模块
├── docs/                     # Documentation / 文档
├── assets/                   # Example/demo assets / 示例/演示资产
├── build*/                   # Build outputs (gitignored) / 构建输出
├── vcpkg/                    # vcpkg package manager / vcpkg 包管理器
├── installer/                # Windows installer / Windows 安装程序
└── scripts/                  # Build and setup scripts / 构建和设置脚本
```

## Source Code / src/

### Core Engine / src/engine/

```
src/engine/
├── audio/                    # Audio system / 音频系统
│   ├── AudioAPI.h/cpp        # Audio API interface
│   ├── AudioBackend.h        # Audio backend abstraction
│   ├── AudioDeviceXAudio2.*  # Windows XAudio2 implementation
│   ├── AudioDeviceSDL3.h     # SDL3 cross-platform backend
│   └── AudioManager.h        # Audio system manager
│
├── core/                     # Core engine components / 核心引擎组件
│   ├── AssetManager.h/cpp    # Asset loading and management
│   ├── AssetBase.h           # Base asset interface
│   ├── Components.h          # ECS component definitions
│   ├── ECS.h/cpp             # Entity Component System
│   └── Systems.h             # System definitions
│
├── graphic/                  # Rendering system / 渲染系统
│   ├── adapters/             # Platform-specific renderers / 平台特定渲染器
│   │   ├── dx12/            # DirectX 12 adapter (Windows)
│   │   │   ├── DX12ResourceFactory.*
│   │   │   └── DX12Backend.*
│   │   └── vulkan/          # Vulkan adapter (Cross-platform)
│   │       └── VulkanShader.h
│   │
│   ├── interfaces/           # Rendering interfaces / 渲染接口
│   │   ├── ICamera.h         # Camera interface
│   │   ├── IPass.h           # Render pass interface
│   │   ├── IResourceFactory.h # Resource creation interface
│   │   ├── IResourceManager.h # Resource management interface
│   │   └── RenderTypes.h     # Common rendering types
│   │
│   ├── pipelines/            # Render pipelines / 渲染管线
│   │   ├── deferred/         # Deferred rendering pipeline
│   │   │   ├── GeometryPass.*
│   │   │   ├── CompositionPass.*
│   │   │   └── DeferredPipeline.*
│   │   └── forward/          # Forward rendering pipeline
│   │       ├── DepthPrePass.*
│   │       ├── ForwardPipeline.*
│   │       ├── OpaquePass.*
│   │       └── TransparentPass.*
│   │
│   ├── ui/                   # UI rendering components
│   │   └── TextRendererComponent.*
│   │
│   ├── Camera.h/cpp          # Camera implementation
│   ├── CameraController.*    # Camera control logic
│   ├── Material.h/cpp        # Material system
│   ├── Mesh.h/cpp            # Mesh geometry
│   ├── RenderComponent.*     # Render component
│   ├── RenderDesc.h          # Render description structures
│   ├── RenderSystemNew.*     # Render system interface
│   └── Shader.h/cpp          # Shader abstraction
│
├── input/                   # Input system / 输入系统
│   └── InputManager.*        # Input management
│
├── math/                    # Mathematics library / 数学库
│   ├── MathTypes.h          # Unified math types (Vector3, Matrix4, etc.)
│   ├── Color.h/cpp          # Color utilities (removed, use MathTypes)
│   ├── MatrixUtils.h        # Matrix helper functions
│   └── Math.h/cpp           # Math functions (removed, use MathTypes)
│
├── platform/                # Platform abstraction / 平台抽象层
│   ├── Platform.h/cpp       # Platform detection
│   ├── PlatformWindows.cpp  # Windows implementation
│   ├── PlatformSDL.cpp      # SDL-based implementation
│   ├── PlatformAndroid.cpp  # Android implementation
│   └── Application.*        # Application interface
│
├── resource/                # Resource management / 资源管理
│   ├── Asset.h/cpp          # Asset base class
│   ├── AssetSerializer.*    # Asset serialization
│   ├── Archive.*            # Archive formats (JSON, binary)
│   ├── TextureAsset.*       # Texture loading
│   ├── MeshAsset.*          # Mesh loading
│   ├── ResourceFallback.*   # Fallback resources
│   └── embedded/            # Embedded resources
│
├── scripting/               # Scripting system / 脚本系统
│   ├── MonoRuntime.*        # Mono/.NET integration
│   └── ScriptSystem.*       # Script management
│
└── [Other core systems]     # Engine, Scene, GameObject, etc.
```

### Launcher / src/launcher/

Platform-specific runtime implementations (similar to Unity Player).

平台特定的运行时实现（类似于 Unity Player）。

```
src/launcher/
├── windows/                 # Windows launcher / Windows 启动器
│   └── WindowsLauncher.cpp  # Windows entry point
│
├── linux/                   # Linux launcher / Linux 启动器
│   └── LinuxLauncher.cpp    # Linux entry point
│
└── android/                 # Android launcher / Android 启动器
    ├── AndroidLauncher.cpp  # Android entry point
    ├── Renderer.*           # Renderer abstraction
    ├── RendererOpenGL.*     # OpenGL rendering
    ├── RendererVulkan.*     # Vulkan rendering
    ├── ShaderOpenGL.*       # OpenGL shaders
    ├── ShaderVulkan.*       # SPIR-V shaders
    ├── VulkanContext.*      # Vulkan context management
    ├── TextureAsset.*       # Texture loading (Android)
    ├── CubemapTextureAsset.* # Cubemap loading
    ├── SkyboxRenderer.*     # Skybox rendering
    ├── Utility.*            # Utility functions
    ├── AndroidOut.*         # Android logging
    ├── renderer/            # Android renderer implementation
    │   ├── API/             # Vulkan API wrappers
    │   ├── RenderPass.*     # Render pass implementations
    │   ├── RenderPipeline.* # Render pipeline
    │   └── TextRenderer.*   # Text rendering
    └── stb_impl.cpp         # STB library implementation
```

### Editor / src/editor/

```
src/editor/
├── Editor.h/cpp             # Main editor application
└── [Editor-specific code]
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
├── SRP2D/                   # 2D Scriptable Render Pipeline 示例
├── Template2D/              # 2D 游戏项目模板（含 C# 脚本）
└── Template3D/              # 3D 游戏项目模板（含 Path Tracing）
```

> **Note**: The Android project (`projects/android/PrismaAndroid/`) has been migrated into the engine's core build system.
>
> **说明**：Android 项目（`projects/android/PrismaAndroid/`）已整合到引擎核心构建系统中。

## Assets / assets/

Example/demo game assets (not part of the engine).

示例/演示游戏资产（不是引擎的一部分）。

```
assets/
├── shaders/                 # Example shaders
├── materials/               # Example materials
└── scenes/                  # Example scenes
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
├── FetchThirdPartyDeps.cmake    # FetchContent dependency management
├── FindMono.cmake               # Mono runtime finder
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
├── Architecture.md           # High-level system design
├── RenderingSystem.md        # Rendering system docs
├── VulkanIntegration.md      # Android/Vulkan integration
├── AssetSerialization.md     # Asset serialization
├── DirectoryStructure.md     # This file
├── ...                       # See Index.md for full list
├── plans/                    # Implementation plans (date-prefixed)
└── superpowers/              # Advanced design specs and plans
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
