# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Prisma Engine (formerly YAGE - Yet Another Game Engine) is a cross-platform game engine built with modern C++20. It supports Windows, Linux, and Android platforms with a focus on modern graphics APIs (DirectX 12, Vulkan).

## 项目概述 / Project Overview (Chinese)

Prisma Engine（原 YAGE - Yet Another Game Engine）是一个使用现代 C++20 构建的跨平台游戏引擎。支持 Windows、Linux 和 Android 平台，专注于现代图形 API（DirectX 12、Vulkan）。

## Build Commands

### Windows Builds
Using CMake presets (recommended):
```bash
# Configure and build x64 Debug
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug

# Configure and build x64 Release
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release

# x86 builds (32-bit)
cmake --preset windows-x86-debug
cmake --build --preset windows-x86-debug

# ARM64 builds (native ARM64 Windows or VS Enterprise only)
cmake --preset windows-arm64-debug
cmake --build --preset windows-arm64-debug
```

Using Visual Studio:
1. Open the PrismaEngine root folder in Visual Studio 2022
2. Visual Studio automatically detects CMake configuration
3. Build solution (Ctrl+Shift+B)

### Linux Builds
```bash
# Note: No preset defined yet, use direct CMake commands
cmake -B build/linux-x64-debug -DCMAKE_BUILD_TYPE=Debug \
      -DPRISMA_ENABLE_RENDER_VULKAN=ON \
      -DPRISMA_ENABLE_RENDER_OPENGL=ON
cmake --build build/linux-x64-debug
```

### Android Builds
```bash
# Using Gradle (in projects/android/PrismaAndroid)
./gradlew assembleDebug

# Or using Android Studio
# Open projects/android/PrismaAndroid as a project
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
│   └── runtime/             # 运行时环境 / Runtime environment
│       ├── windows/         # Windows 运行时
│       ├── linux/           # Linux 运行时
│       └── android/         # Android 运行时
├── resources/               # 引擎资源 / Engine resources
│   ├── common/              # 通用资源 / Common resources
│   │   ├── shaders/
│   │   │   ├── hlsl/        # HLSL 着色器源码
│   │   │   └── glsl/        # GLSL 着色器源码
│   │   ├── textures/        # 通用纹理
│   │   └── fonts/           # 通用字体
│   └── runtime/             # 运行时特定资源 / Runtime-specific resources
│       ├── windows/
│       ├── linux/
│       └── android/
├── projects/                # 平台特定项目 / Platform-specific projects
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
- **ECS (Entity Component System)** architecture / ECS 架构
- `ECS.h` and `Systems.h` in `src/engine/core/`
- `AssetManager` for asset-based resource management
- Transform system for 2D/3D transforms
- ScriptComponent for game logic integration
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
- **C++20 standard**
- **PascalCase** for class names / 类名使用 PascalCase
- **camelCase** for function names / 函数名使用 camelCase
- **Mixed language comments** / 中英文注释混用
- Member variables prefixed with `m_` / 成员变量前缀 `m_`
- **Namespace**: `PrismaEngine` for all engine code / 引擎代码使用 PrismaEngine 命名空间

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
- **Runtime resources** (assets, shaders) go to `resources/runtime/{platform}/`
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
  - `src/runtime/{platform}/`

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

## Android Development / Android 开发

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
- Vulkan renderer on Linux needs testing / Linux 上的 Vulkan 渲染器需要测试
- XAudio2 backend disabled pending interface refactoring / XAudio2 后端暂时禁用，等待接口重构
- Script system (Mono) not fully implemented / 脚本系统（Mono）未完全实现
- Physics engine integration pending / 物理引擎集成待定
- Linux CMake presets not defined (use manual CMake commands) / Linux CMake 预设未定义（需手动使用 CMake 命令）

## Related Documentation / 相关文档
- [Rendering System](docs/RenderingSystem.md)
- [Asset Management](docs/AssetSerialization.md)
- [Android Integration](docs/VulkanIntegration.md)
- [Architecture](docs/README_zh.md)
