# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Prisma Engine (formerly YAGE - Yet Another Game Engine) is a cross-platform game engine built with modern C++23. It supports Windows, Linux, and Android platforms with a focus on modern graphics APIs (DirectX 12, Vulkan).

## 项目概述 / Project Overview (Chinese)

Prisma Engine（原 YAGE - Yet Another Game Engine）是一个使用现代 C++23 构建的跨平台游戏引擎。支持 Windows、Linux 和 Android 平台，专注于现代图形 API（DirectX 12、Vulkan）。

## Build Commands

### Windows Builds
Using CMake presets (recommended):
```bash
# 1. Configure (creates Visual Studio solution in build/windows-x64-debug)
cmake --preset windows-x64-debug

# 2. Build all targets (from the build directory directly)
cmake --build build/windows-x64-debug

# Or build specific targets only
cmake --build build/windows-x64-debug --target Engine
cmake --build build/windows-x64-debug --target Editor
cmake --build build/windows-x64-debug --target Launcher
cmake --build build/windows-x64-debug --target PathTracing3D

# Release
cmake --preset windows-x64-release
cmake --build build/windows-x64-release
```

Using Visual Studio:
1. Open the PrismaEngine root folder in Visual Studio 2026
2. Visual Studio automatically detects CMake configuration (opens build/windows-x64-debug)
3. Build solution (Ctrl+Shift+B)

### Linux Builds
Using CMake presets:
```bash
# 1. Configure
cmake --preset linux-x64-debug

# 2. Build all
cmake --build build/linux-x64-debug

# Release
cmake --preset linux-x64-release
cmake --build build/linux-x64-release

# ARM64 cross-compile
cmake --preset linux-arm64-debug
cmake --build build/linux-arm64-debug
```

### Android Builds
Using CMake presets:
```bash
# Engine
cmake --preset engine-android-arm64-debug
cmake --build --preset engine-android-arm64-debug

cmake --preset engine-android-arm64-release
cmake --build --preset engine-android-arm64-release

# Launcher
cmake --preset launcher-android-arm64-debug
cmake --build --preset launcher-android-arm64-debug

cmake --preset launcher-android-arm64-release
cmake --build --preset launcher-android-arm64-release
```

Using Gradle:
```bash
# In projects/android/PrismaAndroid
./gradlew assembleDebug

# Or using Android Studio
# Open projects/android/PrismaAndroid as a project
```


### WebAssembly Builds
```bash
# Requires emsdk + emcmake
emcmake cmake -B build/launcher-web-debug -DPRISMA_BUILD_EDITOR=OFF \
              -DPRISMA_BUILD_SHARED_LIBS=OFF -DPRISMA_LAUNCHER_DYNAMIC_LOAD=OFF
cmake --build build/launcher-web-debug --target Launcher
```

### C# Scripting Builds (Cross-Platform)
```bash
# General build
dotnet build projects/Prisma2D/scripts/GameScripts/GameScripts.csproj
dotnet build projects/PathTracing3D/scripts/GameScripts.csproj

# Termux / Restricted VM Build (CRITICAL)
# Android/Termux environments often limit virtual memory (ulimit -v). 
# .NET 10+ defaults to a large GC region reservation (256GB) which will fail.
# Use this environment variable to cap reservation to 256MB:
export DOTNET_GCRegionRange=0x10000000 && dotnet build projects/Prisma2D/scripts/GameScripts/GameScripts.csproj
```

### Environment Setup
```bash
# Initialize vcpkg
./vcpkg/bootstrap-vcpkg.bat  # Windows
./vcpkg/bootstrap-vcpkg.sh   # Linux/macOS

# Install dependencies (if using vcpkg mode)
./vcpkg/vcpkg install
```

**Note**: The project uses CMake FetchContent by default for dependency management. Set `PRISMA_USE_FETCHCONTENT=OFF` to use vcpkg instead.

## Architecture Overview

### Project Structure / 项目结构

```
PrismaEngine/
├── src/
│   ├── engine/              # 核心引擎代码 / Core engine code
│   │   ├── audio/           # 音频系统 / Audio system
│   │   ├── core/            # 核心组件 / Core components (ECS, Asset)
│   │   ├── graphic/         # 渲染系统 / Rendering system
│   │   │   ├── adapters/    # 渲染后端适配器 / Renderer adapters
│   │   │   │   ├── dx12/    # DirectX 12 实现
│   │   │   │   └── vulkan/  # Vulkan 实现
│   │   │   ├── pipelines/   # 渲染管线 / Render pipelines
│   │   │   └── interfaces/  # 渲染接口 / Rendering interfaces
│   │   ├── input/           # 输入系统 / Input system
│   │   ├── math/            # 数学库 / Math library
│   │   ├── platform/        # 平台抽象层 / Platform abstraction
│   │   ├── resource/        # 资源管理 / Resource management
│   │   └── scripting/       # 脚本系统 / Scripting system
│   ├── editor/              # 编辑器应用 / Editor application
│   ├── game/                # 游戏框架 / Game framework
│   └── launcher/             # 启动器 / Launcher
│       ├── windows/         # Windows 启动器
│       ├── linux/           # Linux 启动器
│       ├── android/         # Android 启动器
│       └── web/             # WebAssembly 启动器
├── resources/               # 引擎资源 / Engine resources
│   ├── common/              # 通用资源 / Common resources
│   │   ├── shaders/
│   │   │   ├── hlsl/        # HLSL 着色器源码
│   │   │   └── glsl/        # GLSL 着色器源码
│   │   ├── textures/        # 通用纹理
│   │   └── fonts/           # 通用字体
│   └── launcher/             # 启动器特定资源 / Launcher-specific resources
│       ├── windows/
│       ├── linux/
│       └── android/
├── projects/                # 项目模板 / Project templates
│   ├── Prisma2D/          # 2D 场景模板（C# 脚本驱动）
│   ├── PathTracing3D/          # 3D 路径追踪模板
│   │   ├── src/             # C++: StatsOverlay, HeadlessRunner, PathTracing3DApp
│   │   └── scripts/         # C#: CameraController3D, ScriptEntry
│   ├── PrismaCraft/         # Minecraft 复刻项目
│   └── android/             # Android Studio 项目
├── cmake/                   # CMake 模块 / CMake modules
├── docs/                    # 文档 / Documentation
└── vcpkg.json               # vcpkg 依赖配置
```

### Core Systems / 核心系统

#### Rendering System / 渲染系统
- **DirectX 12**: Primary Windows rendering backend / Windows 主要渲染后端
- **Vulkan**: Cross-platform support (Windows, Linux, Android) / 跨平台支持
- **OpenGL**: Fallback backend (Linux)
- Render backend interfaces in `src/engine/graphic/interfaces/`
- Implementations in `src/engine/graphic/adapters/`
- **Render Pipelines**: Forward and Deferred in `src/engine/graphic/pipelines/`
- **Shader Support**:
  - HLSL for DirectX 12 (`resources/common/shaders/hlsl/`)
  - GLSL for Vulkan/OpenGL (`resources/common/shaders/glsl/`)
  - Android: Automatic GLSL→SPIR-V compilation via Gradle

#### Audio System / 音频系统
- **XAudio2**: Windows native backend (currently disabled, needs interface rewrite)
- **SDL3**: Cross-platform backend in `src/engine/audio/AudioDeviceSDL3.*`
- **AAudio**: Android native backend (planned)
- Located in `src/engine/audio/`

Note: XAudio2 implementation exists in `audio/AudioDeviceXAudio2.*` and `audio/drivers/AudioDriverXAudio2.*` but is currently commented out in CMakeLists.txt pending interface refactoring.

#### Resource Management / 资源管理
- **Asset-based system** / 基于资产的系统
- `AssetManager` in `src/engine/core/AssetManager.*`
- JSON and binary serialization support / JSON 和二进制序列化支持
- Thread-safe loading / 线程安全加载
- **Asset Types**:
  - `TextureAsset`: Texture loading with STB
  - `MeshAsset`: 3D mesh data
  - `TilemapAsset`: 2D tilemap support (Tiled TMX format)
  - `CubemapTextureAsset`: Skybox/environment textures
- **Format Support**:
  - JSON-based asset serialization
  - Base64+zstd compressed binary format
  - TMX (Tiled Map) XML format via tinyxml2

#### Component System / 组件系统
- **Component-based** architecture
- `Component` base class in `src/engine/core/Component.h`
- `ComponentRegistry` for factory-based serialization
- Key components: `Transform`, `Camera`, `PrimitiveComponent`, `MeshRenderer`, `SpriteRenderer`, `ScriptComponent`
- **UI System**: 2D UI components in `src/engine/ui/`
  - `UIComponent`: Base UI component
  - `ButtonComponent`, `CanvasComponent`: 2D UI elements
  - `UIInputManager`: Input handling for UI

### Key Dependencies / 主要依赖
Managed via CMake FetchContent (default) or vcpkg:
- **DirectX-Headers**: DirectX 12 support (Windows)
- **Vulkan-Headers**: Vulkan API headers
- **VMA (Vulkan Memory Allocator)**: Vulkan memory management
- **vk-bootstrap**: Vulkan initialization helper
- **SDL3**: Windowing, input, and audio abstraction
- **ImGui**: Editor UI framework
- **nlohmann-json**: JSON serialization
- **GLM**: Mathematics library (cross-platform)
- **stb**: Image loading and text rendering
- **tinyxml2**: XML parsing (for TMX tilemap format)
- **zstd**: Compression library (for asset formats)
- **game-activity**: Android native app framework

Dependencies are configured in `cmake/FetchThirdPartyDeps.cmake`.

## Development Guidelines / 开发指南

### Code Style / 代码风格
- **C++23 standard**
- **PascalCase** for class names / 类名使用 PascalCase
- **camelCase** for function names / 函数名使用 camelCase
- **Mixed language comments** / 中英文注释混用
- Member variables prefixed with `m_` / 成员变量前缀 `m_`
- **Namespace**: `PrismaEngine` for all engine code / 引擎代码使用 PrismaEngine 命名空间

### Warning Rules / 警告处理规则
- **禁止使用 `(void)` 消除未引用参数/变量警告** — 优先判断参数是否有语义价值。有意义的参数保留 + `[[maybe_unused]]`，无意义才删参数名
- **未引用参数如果来自空函数，应添加真实逻辑而不是仅打 LOG_DEBUG** — 函数必须有实际行为（如 Platform 窗口函数应实现真实窗口操作）；完全死代码（无外部引用）可以直接删除
- 第三方库（VMA、glaze、SPIRV-Tools 等）的内部警告用宏压制（`#pragma warning` 或 CMake `/wd`），不得修改库源码

### Conditional Compilation / 条件编译
Engine modules are conditionally compiled based on platform and feature flags:

```cpp
// Check if rendering backend is enabled
#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    // Vulkan-specific code
#endif

// Check if audio device is enabled
#if defined(PRISMA_ENABLE_AUDIO_SDL3)
    // SDL3 audio code
#endif
```

See `cmake/DeviceOptions.cmake` for all available `PRISMA_ENABLE_*` options.

### Namespace Convention / 命名空间约定
```cpp
namespace PrismaEngine {
    namespace Graphic {
        // Rendering-related code
    }
    namespace Audio {
        // Audio-related code
    }
}
```

### Module Integration / 模块集成
When adding new systems:
1. Create interface class in appropriate `src/engine/*/interfaces/`
2. Implement in platform-specific adapters (dx12, vulkan, etc.)
3. Register with EngineCore system manager
4. Add to `src/engine/CMakeLists.txt` with conditional compilation (`PRISMA_ENABLE_*` options)

### Native vs Cross-Platform Mode / 原生 vs 跨平台模式
The engine supports two modes configured via CMake options:

**Native Mode** (default for rendering):
- Windows: DirectX 12, XAudio2, XInput
- Android: Vulkan, AAudio, GameActivity
- Linux: OpenGL/Vulkan (in development)

**Cross-Platform Mode** (SDL3-based):
- Audio: SDL3 audio backend
- Input: SDL3 input backend
- App: SDL3 main entry

Configure via:
```bash
-DPRISMA_USE_NATIVE_AUDIO=OFF    # Use SDL3 audio instead of native
-DPRISMA_USE_NATIVE_INPUT=OFF    # Use SDL3 input instead of native
```

### Resource Management Guidelines / 资源管理指南
- **Launcher resources** (assets, shaders) go to `resources/launcher/{platform}/`
- **Common resources** (shared across platforms) go to `resources/common/`
- **Editor resources** go to `resources/editor/`
- Use `AssetManager` for loading all runtime assets
- Android: Place runtime assets in `projects/android/PrismaAndroid/app/src/main/assets/`
- Android shaders are auto-compiled from `assets/shaders/*.vert`, `*.frag` to SPIR-V

### Platform-Specific Code / 平台特定代码
- Use platform abstraction layer in `src/engine/platform/`
- Avoid direct platform APIs in core engine code
- Implement platform-specific versions in:
  - `src/engine/platform/Platform{Windows,Linux,Android}.cpp`
  - `src/launcher/{platform}/`

## CMake Configuration Options / CMake 配置选项

### Key CMake Options / 主要 CMake 选项

| Option | Description | Default |
|--------|-------------|---------|
| `PRISMA_BUILD_EDITOR` | Build Editor application | ON (Windows only) |
| `PRISMA_BUILD_SHARED_LIBS` | Build Engine as shared library | ON (Debug), OFF (Release) |
| `PRISMA_USE_FETCHCONTENT` | Use FetchContent for dependencies | ON |
| `PRISMA_ENABLE_RENDER_DX12` | Enable DirectX 12 backend | ON (Windows) |
| `PRISMA_ENABLE_RENDER_VULKAN` | Enable Vulkan backend | ON (Android), OFF (Windows) |
| `PRISMA_ENABLE_RENDER_OPENGL` | Enable OpenGL backend | OFF |
| `PRISMA_ENABLE_AUDIO_XAUDIO2` | Enable XAudio2 audio | ON (Windows, but disabled pending refactor) |
| `PRISMA_ENABLE_AUDIO_SDL3` | Enable SDL3 audio | ON (cross-platform) |
| `PRISMA_ENABLE_IMGUI_DEBUG` | Enable ImGui debug UI | ON (Debug builds) |
| `PRISMA_USE_NATIVE_AUDIO` | Use platform native audio APIs | ON |
| `PRISMA_USE_NATIVE_INPUT` | Use platform native input APIs | ON |

### Upscaler Options / 超分辨率选项
Configured in `cmake/UpscalerOptions.cmake`:
- `PRISMA_ENABLE_UPSCALER_FSR`: AMD FSR 2.2 support
- `PRISMA_ENABLE_UPSCALER_DLSS`: NVIDIA DLSS support
- `PRISMA_ENABLE_UPSCALER_TSR`: Unreal TSR-style upscaler

All upscalers are disabled by default.

## CI/CD

CI 自动在 `main`/`develop`/`dev` 分支 Push/PR 时触发：

| Platform | Workflow |
|----------|----------|
| Windows  | `ci-windows.yml` (VS 2022, 只构建引擎) |
| Linux    | `ci-linux.yml` (Ninja) |
| Android  | `ci-android.yml` (NDK r28, API 34) |

## Android Development / Android 开发

- **API Level**: 34 (Vulkan 1.3+)
- **NDK**: r28

### Directory Structure / 目录结构
```
projects/android/PrismaAndroid/
├── app/
│   ├── src/main/
│   │   ├── cpp/          # JNI glue code
│   │   ├── java/         # Android Activity
│   │   ├── assets/       # Runtime assets (copied from resources/)
│   │   └── res/          # Android resources (icons, etc.)
│   └── build.gradle.kts  # Gradle build config
└── ...
```

### Shader Compilation / 着色器编译
Android Gradle Plugin **automatically compiles** GLSL shaders to SPIR-V:
- Source: `app/src/main/assets/shaders/**/*.vert`, `**.frag`
- Output: `build/intermediates/shader_assets/**/*.spv`
- Included in APK automatically

### Asset Loading / 资源加载
```cpp
// Load shader SPIR-V
auto vertShaderCode = ShaderVulkan::loadShader(
    assetManager, "shaders/skybox.vert.spv"
);

// Load texture
auto texture = TextureAsset::loadAsset(
    assetManager, "textures/android_robot.png"
);
```

## Known Limitations / 已知限制
- Vulkan RT backend uses VK 1.2 functions (loaded dynamically on Android)
- XAudio2 backend disabled pending interface refactoring
- Linux CMake presets defined but not fully tested

## Related Documentation / 相关文档
- [Rendering System](docs/RenderingSystem.md)
- [Asset Management](docs/AssetSerialization.md)
- [Android Integration](docs/VulkanIntegration.md)
- [Architecture](docs/README_zh.md)
