# Prisma Engine Project Context

This file provides critical context and instructions for AI agents working on the Prisma Engine codebase.

## Project Overview

Prisma Engine is a high-performance, cross-platform 3D game engine built with **modern C++23**, focusing on a **Vulkan** rendering backend and **SDL3** for platform abstraction (input, audio, window management). It utilizes a modular architecture with a **Driver-Device pattern** to decouple high-level logic from platform-specific APIs.

### Core Technologies
- **Language:** C++23 (Concepts, Coroutines, Designated Initializers)
- **Graphics API:** Vulkan (Primary, cross-platform), DirectX 12 (Windows-specific, secondary)
- **Platform Layer:** SDL3
- **Scripting:** CoreCLR (.NET 10) primary, Mono legacy
- **UI:** ImGui (Editor and Debug tools), WebUI (Browser-based editor)
- **Build System:** CMake (3.31+) with Presets, FetchContent dependency management

### Architecture
- **Driver-Device Pattern:** Decouples platform-specific implementations (Drivers) from high-level engine services (Devices).
- **ECS (Entity Component System):** High-performance object management.
- **Render Pipeline:** Feature-based architecture supporting Forward and Deferred paths.
- **Asset Management:** Handle-based system with generational indices for type safety and memory efficiency.
- **Modern Design Philosophy:** Explicitly avoids legacy patterns like Unity's Interop overhead, Reflection-based magic, and GC pressure (see `docs/UnityLegacyAvoidance.md`).

## Directory Structure
- `src/engine/`: Core engine source code (Audio, Graphic, ECS, Input, Scripting, etc.)
- `src/editor/`: WebUI editor, ImGui panels, MCP server, editor core.
- `src/launcher/`: Cross-platform runtime launcher entry point.
- `src/tests/`: Engine tests (AudioTest).
- `projects/`: Sample games and templates (Prisma2D, PathTracing3D, PacManGame, PrismaCraft, ClusteredForward3D, Deferred3D, SRP2D, NeoEditor).
- `sdk/`: Public headers + CMake config for `find_package(PrismaEngine)`.
- `assets/shaders/`: Shader source files (GLSL, PBR/NPR/clustered).
- `.dependencies/`: Vendored third-party libraries (FetchContent cache).
- `scripts/`: Build, package, and utility scripts.
- `docs/`: Extensive documentation on systems and architecture.
- `cmake/`: Modular CMake configuration files (Engine/Editor/Launcher/SDK targets).

## Building and Running

### Prerequisites
- CMake 3.31+
- C++23 compatible compiler (MSVC 17.10+, GCC 13+, Clang 16+)
- Vulkan SDK
- SDL3

### Key Commands
The engine uses CMake presets directly:

```bash
# Configure with preset
cmake --preset linux-x64-debug

# Build everything
cmake --build build/linux-x64-debug --parallel

# Build specific target
cmake --build build/linux-x64-debug --target Engine

# Build a project
cmake --build build/linux-x64-debug --target PathTracing3D
```

### CMake Presets (see CMakePresets.json)
Common presets include:
- `linux-x64-debug` / `linux-x64-release`
- `windows-x64-debug` / `windows-x64-release`
- `linux-arm64-debug` (for ARM targets)
- Android builds configured via Gradle + CMake

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
    - C++23, no `(void)` casts — use `[[maybe_unused]]`.
    - Namespace `PrismaEngine { namespace Graphic { ... } }` for engine code.
    - Third-party warnings suppressed via CMake flags, never by modifying library source.

### Key Subsystems
- **Audio:** DSP node graph (22 node types), multi-backend (miniaudio/SDL3/XAudio2), acoustic raytracing.
- **Input:** `InputDevice` uses `IInputDriver` (SDL3, Win32, GameActivity), enhanced input manager.
- **Graphics:** Unified RHI with Vulkan backend, Forward/Deferred/Clustered/NPR/PathTracing pipelines.
- **Resources:** Handle&lt;T&gt; generational index system, ResourcePool free-list.
- **Scripting:** CoreCLR .NET 10 self-hosted, C# Prisma.Core library with SRP bindings.
- **MCP:** Model Context Protocol server (17 tools, 7 categories, dual transport).

## Engineering Roadmap & TODOs

Refer to [docs/Roadmap.md](docs/Roadmap.md) and [docs/Index.md](docs/Index.md) for current engineering priorities, module status, and implementation plans.

## Guidelines for AI Agents
1. **Always use `-j2` or `-j4` for builds** on ARM/limited resources to avoid memory exhaustion (as per global context).
2. **Follow the Driver-Device pattern** when adding platform-specific features.
3. **Update `docs/Roadmap.md`** (Module Progress section) when completing or modifying major features.
4. **Refer to `docs/`** for detailed specifications of individual systems before refactoring.
5. **Check `CMakePresets.json`** for environment-specific configurations before suggesting build fixes.
6. **Environment Optimization (Encoding)**: 在 Windows 中文环境下，建议 AI Agent 在执行构建命令时带上英文语言包前缀，以确保日志可读：
    - 组合命令：`$env:VSLANG = '1033'; $env:DOTNET_CLI_UI_LANGUAGE = 'en-US'; chcp 437; ./scripts/build-windows.bat`
