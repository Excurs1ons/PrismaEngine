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

### Runtime / src/runtime/

Platform-specific runtime implementations (similar to Unity Player).

平台特定的运行时实现（类似于 Unity Player）。

```
src/runtime/
├── windows/                 # Windows runtime / Windows 运行时
│   └── WindowsRuntime.cpp   # Windows entry point
│
├── linux/                   # Linux runtime / Linux 运行时
│   └── LinuxRuntime.cpp     # Linux entry point
│
└── android/                 # Android runtime / Android 运行时
    ├── AndroidRuntime.cpp   # Android entry point
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

### Common Resources / resources/common/

Shared resources across runtime and editor.

Runtime 和 Editor 共用的资源。

```
resources/common/
├── shaders/                 # Shader source code / 着色器源码
│   ├── hlsl/                # HLSL shaders (for DX12)
│   │   ├── default.hlsl     # Default shader
│   │   ├── Skybox.hlsl      # Skybox shader
│   │   └── Text.hlsl        # Text rendering shader
│   │
│   └── glsl/                # GLSL shaders (for Vulkan/OpenGL)
│       ├── clearcolor.vert/frag
│       ├── shader.vert/frag
│       └── skybox.vert/frag
│
├── textures/                # Common textures / 通用纹理
│   └── android_robot.png    # Example texture
│
└── fonts/                   # Common fonts / 通用字体
```

### Runtime Resources / resources/runtime/

Platform-specific runtime resources (icons, etc.).

平台特定的运行时资源（图标等）。

```
resources/runtime/
├── windows/
│   └── icons/               # Windows icons
│       ├── Runtime.ico      # Main application icon
│       └── small.ico        # Small icon
│
├── linux/
│   └── icons/               # Linux icons
│
└── android/
    └── icons/               # Android icons
```

### Editor Resources / resources/editor/

Editor-specific resources (UI icons, themes, etc.).

编辑器专用资源（UI 图标、主题等）。

```
resources/editor/
├── icons/                   # Editor UI icons
├── fonts/                   # Editor fonts
├── themes/                  # Editor themes
└── layouts/                 # Editor layouts
```

## Platform Projects / projects/

### Android Project / projects/android/PrismaAndroid/

Android Studio project for Android runtime.

Android 运行时的 Android Studio 项目。

```
projects/android/PrismaAndroid/
├── app/
│   ├── src/main/
│   │   ├── cpp/             # Native C++ code / 原生 C++ 代码
│   │   │   └── CMakeLists.txt
│   │   ├── java/            # Java/Kotlin code
│   │   │   └── MainActivity.java
│   │   ├── assets/          # Runtime assets (copied during build)
│   │   │   ├── shaders/     # Compiled SPIR-V shaders
│   │   │   └── textures/    # Runtime textures
│   │   ├── res/             # Android resources
│   │   │   ├── drawable/
│   │   │   ├── mipmap-*/    # App icons
│   │   │   └── values/      # Resource values
│   │   └── AndroidManifest.xml
│   └── build.gradle.kts     # Gradle build configuration
│
└── [Gradle config files]
```

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
├── FetchThirdPartyDeps.cmake  # FetchContent dependency management
├── DeviceOptions.cmake         # Device/platform-specific options
└── [Other CMake modules]
```

## Documentation / docs/

```
docs/
├── CLAUDE.md                 # Project overview (this file)
├── README_zh.md              # Chinese documentation index
├── RenderingSystem.md        # Rendering system docs
├── AssetSerialization.md     # Asset serialization
├── VulkanIntegration.md      # Android/Vulkan integration
└── [Other documentation]
```

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
1. Create implementation in `src/runtime/{platform}/` / 在运行时目录创建实现
2. Use platform abstraction in `src/engine/platform/` / 使用平台抽象层
3. Add conditional compilation if needed / 如需要使用条件编译

### Adding Resources / 添加资源
- **Common shaders**: `resources/common/shaders/hlsl/` or `glsl/`
- **Common textures**: `resources/common/textures/`
- **Platform-specific**: `resources/runtime/{platform}/`

## Related Documentation / 相关文档
- [CLAUDE.md](../CLAUDE.md) - Project overview / 项目概述
- [Rendering System](RenderingSystem.md) - Rendering architecture / 渲染架构
- [Vulkan Integration](VulkanIntegration.md) - Android/Vulkan setup
