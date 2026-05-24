# Prisma Engine Project Context

This file provides critical context and instructions for AI agents working on the Prisma Engine codebase.

## Project Overview

Prisma Engine is a high-performance, cross-platform 3D game engine built with **modern C++20**, focusing on a **Vulkan** rendering backend and **SDL3** for platform abstraction (input, audio, window management). It utilizes a modular architecture with a **Driver-Device pattern** to decouple high-level logic from platform-specific APIs.

### Core Technologies
- **Language:** C++20 (Concepts, Coroutines, Designated Initializers)
- **Graphics API:** Vulkan (Primary), DirectX 12 (Windows-specific, secondary)
- **Platform Layer:** SDL3
- **Scripting:** Mono / CoreCLR (.NET)
- **UI:** ImGui (Editor and Debug tools)
- **Build System:** CMake (3.31+) with Presets

### Architecture
- **Driver-Device Pattern:** Decouples platform-specific implementations (Drivers) from high-level engine services (Devices).
- **ECS (Entity Component System):** High-performance object management.
- **Render Pipeline:** Feature-based architecture supporting Forward and Deferred paths.
- **Asset Management:** Handle-based system with generational indices for type safety and memory efficiency.
- **Modern Design Philosophy:** Explicitly avoids legacy patterns like Unity's Interop overhead, Reflection-based magic, and GC pressure (see `docs/UnityLegacyAvoidance.md`).

## Directory Structure
- `src/engine/`: Core engine source code (Audio, Graphic, ECS, Input, etc.)
- `src/editor/`: ImGui-based editor tools.
- `src/runtime/`: Engine runtime executable.
- `projects/`: Sample games and templates (e.g., `PacManGame`, `Prisma2D`).
- `scripts/`: Build, package, and utility scripts.
- `docs/`: Extensive documentation on systems and architecture.
- `cmake/`: Modular CMake configuration files.

## Building and Running

### Prerequisites
- CMake 3.31+
- C++20 compatible compiler (MSVC 17.10+, GCC 11+, Clang 13+)
- Vulkan SDK
- SDL3

### Key Commands
The engine uses a unified build script for automation:

```bash
# Build the engine (default: debug)
./scripts/build.sh --target engine

# Build and run PacMan sample game
./scripts/run-pacman.sh --config debug

# Build the editor
./scripts/build.sh --target editor --config release

# Clean build directory
./scripts/build.sh --clean
```

### CMake Presets
Common presets include:
- `engine-linux-x64-debug` / `engine-linux-x64-release`
- `editor-linux-x64-debug` / `editor-linux-x64-release`
- `pacman-linux-x64-debug` / `pacman-linux-x64-release`
- `engine-android-arm64-debug` / `engine-android-arm64-release`

## Development Conventions

### Coding Style
- **Naming:**
    - Files: `PascalCase.h`, `PascalCase.cpp`
    - Classes/Types: `PascalCase`
    - Interfaces: `IPascalCase`
    - Member Variables: `m_camelCase`
    - Local Variables: `camelCase`
- **Formatting:**
    - Braces: Allman style (braces on new lines).
    - Indentation: 4 spaces (no tabs).
- **Standards:**
    - No `using namespace std;`.
    - Prefer `smart pointers` and `RAII` over manual memory management.
    - Use `static_cast` instead of C-style casts.

### Key Subsystems
- **Audio:** `AudioDevice` uses `IAudioDriver` (SDL3, XAudio2, AAudio).
- **Input:** `InputDevice` uses `IInputDriver` (SDL3, Win32, GameActivity).
- **Graphics:** `Renderer` and `RenderGraph` manage the Vulkan pipeline.
- **Resources:** `AssetManager` handles loading via `AssetSerializer`.

## Engineering Roadmap & TODOs

### 🔴 高优先级 (High Priority)
- **运行时着色器编译 (Runtime Shader Compilation)**: 
    - 引入 `shaderc` (Google) 或 `DXC` (Microsoft) 作为引擎内部库依赖。
    - 实现 `IShader::RecompileFromSource` 接口，支持 GLSL/HLSL 直接编译为 SPIR-V。
    - 移除对外部 `glslc` 或 `dxc.exe` 的硬性环境依赖，提升工程便携性。
- **Glaze 迁移与 nlohmann/json 清理**:
    - 完成从 `nlohmann/json` 到 `Glaze` 的全面迁移。 (已完成构建系统和核心库重构)
    - 移除所有源文件中对 `nlohmann/json.hpp` 的引用。 (已完成)
- **Visual Studio 2026 兼容性维护**:
    - 确保后续新增模块在 MSVC v144 编译器下的零警告编译。

### 🟡 中优先级 (Medium Priority)
- **依赖便携化 (Dependency Portability)**:
    - 将 Vulkan Loader 和 .NET SDK 绿色化，放入 `.dependencies` 目录。
    - 更新 `setup-env.ps1` 支持自动下载上述便携组件。

## Guidelines for AI Agents
1. **Always use `-j2` or `-j4` for builds** on ARM/limited resources to avoid memory exhaustion (as per global context).
2. **Follow the Driver-Device pattern** when adding platform-specific features.
3. **Update `docs/Roadmap.md`** (Module Progress section) when completing or modifying major features.
4. **Refer to `docs/`** for detailed specifications of individual systems before refactoring.
5. **Check `CMakePresets.json`** for environment-specific configurations before suggesting build fixes.
6. **Environment Optimization (Encoding)**: 在 Windows 中文环境下，建议 AI Agent 在执行构建命令时带上英文语言包前缀，以确保日志可读：
    - 组合命令：`$env:VSLANG = '1033'; $env:DOTNET_CLI_UI_LANGUAGE = 'en-US'; chcp 437; ./scripts/build-windows.bat`
