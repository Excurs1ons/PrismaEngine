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
- `projects/`: Sample games and templates (e.g., `PacManGame`, `Template2D`).
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

## Guidelines for AI Agents
1. **Always use `-j2` or `-j4` for builds** on ARM/limited resources to avoid memory exhaustion (as per global context).
2. **Follow the Driver-Device pattern** when adding platform-specific features.
3. **Update `docs/MODULE_PROGRESS.md`** when completing or modifying major features.
4. **Refer to `docs/`** for detailed specifications of individual systems before refactoring.
5. **Check `CMakePresets.json`** for environment-specific configurations before suggesting build fixes.
