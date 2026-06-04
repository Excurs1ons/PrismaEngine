# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![CI Windows](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml)
[![CI Linux](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml)
[![CI Android](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml)
[![Android APK](https://img.shields.io/badge/APK-Download-green.svg?logo=android)](https://github.com/Excurs1ons/PrismaEngine/releases/download/latest/PrismaAndroid.apk)
[![CodeWiki](https://img.shields.io/badge/CodeWiki-Google-4285F4?style=flat-square)](https://codewiki.google/github.com/Excurs1ons/PrismaEngine)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Excurs1ons/PrismaEngine)
[![Mintlify](https://img.shields.io/badge/Mintlify-Docs-262626?style=flat-square&logo=googlegemini&logoColor=fff)](https://mintlify.wiki/Excurs1ons/PrismaEngine)

Prisma Engine is a cross-platform 3D game engine built with modern C++23, focusing on high-performance rendering and modern graphics architectures.

[中文文档](./docs/README_zh.md) | [English](./README.md)

> **Current Status**: CoreCLR C# scripting production-ready. MetroidvaniaDemo (NES-style, 16 C# scripts) fully playable. 2D platformer physics + audio zones + pixel-perfect pipeline complete. SoA entity pool with 1M virtual capacity. Android Vulkan runtime production-ready.
> **Last Updated**: 2026-06-04

## CI/CD Status

| Platform | Status | Trigger |
|----------|--------|---------|
| **Windows** | [![CI Windows](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml) | Push / PR |
| **Linux** | [![CI Linux](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml) | Push / PR |
| **Android** | [![CI Android](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml) | Push / PR |

## Current Progress

| Module | Status | Description |
|--------|--------|-------------|
| AI System | ✅ 75% | Behavior tree, state machine, perception system, goal-oriented |
| Android Runtime | ✅ 90% | Optimized Vulkan runtime (integrated GameActivity) |
| Animation System | ✅ 70% | Skeletal animation, blend tree, IK, morph targets, SpriteAnimation batched |
| Audio System | ✅ 80% | XAudio2/SDL3/miniaudio backends, 12 DSP effects, Flac/Mp3/Ogg codecs, 2D zone audio |
| 2D Lighting | ✅ 85% | 2D shadow mapping, Point/Spot lights, ambient color, C# Light2D manager |
| 2D Rendering | ✅ 85% | 2D lighting, shadow geometry, post-processing (Bloom/Grayscale/Distortion), tilemap GPU instancing, sprite animation batching, PixelPerfect + CRT |
| 2D Platform Physics | ✅ 95% | AABB sweep test, ResolvePlatform (5-iteration), one-way platform, quadtree spatial acceleration, raycast, trigger volumes |
| Audio System | ✅ 85% | XAudio2/SDL3/miniaudio backends, 12 DSP effects, Flac/Mp3/Ogg codecs, 2D zone audio with BGM crossfade |
| Console/CVar | ✅ 70% | In-game console, CVar system, auto-complete, history |
| CoreCLR Scripting | ✅ 95% | C# Node/Script + SRP (CommandBuffer, RendererFeature, Compute Pipeline), 16-gameplay scripts in MetroidvaniaDemo |
| DirectX 12 Backend | ❌ Suspended | Suspended, focusing on Vulkan backend |
| Editor Tools | ✅ 70% | ImGui editor panels, MCP tools, ComponentSerializer, scene save/load, WebUI |
| Input System | ✅ 90% | Keyboard/mouse/gamepad, action mapping, chord detection, simulated input scripting, event forwarding to C# |
| Logger System | ✅ 100% | Thread-safe cross-platform logging |
| MCP Protocol | ✅ 95% | 17 tools, 7 categories, dual transport, hash delta tracking |
| Memory System | ✅ 90% | Stack/arena/pool allocators, virtual memory, double-buffered |
| Navigation System | ✅ 90% | NavMesh generation, A* pathfinding, nav agent, avoidance, binary serialization |
| Network System | ✅ 70% | TCP/UDP transport, RPC, entity replication, lobby |
| Particle System | ✅ 75% | CPU/GPU particles + GPU compute pipeline, emitters, force fields, events, 2D projection |
| Physics System | ✅ 85% | Collision detection, rigid body, constraint solver, CCD, trigger volumes, 2D quadtree |
| Platform Layer | ✅ 95% | Unified Windows/Linux/Android abstraction |
| Post-Processing | ✅ 85% | Bloom/Grayscale/Distortion + PixelPerfect + CRT, Pipeline2D integrated |
| Profiling System | ✅ 60% | GPU/CPU profiler, frame stats, timeline capture |
| Rendering Architecture | ✅ 95% | Core Pass + Feature system + Compute Pipeline RHI |
| Resource Management | ✅ 95% | Handle\<T\> system + resource pools |
| Shaders | ✅ 78% | PBR lighting, shadow mapping, SSAO, TAA, IBL, deferred/clustered/NPR/SSR/Fog/Tonemap shaders |
| SoA Entity Pool | ✅ 100% | Virtual memory 1M capacity, double-buffered |
| Terrain System | ✅ 80% | Heightmap, LOD, chunk-based loading, splatmap texturing, Vulkan rendering |
| Tilemap | ✅ 85% | GPU instancing, JSON map loading, collision data export, C# API binding |
| Testing Infrastructure | ✅ 65% | GTest unit tests + integration tests (70 files, CI-enabled) |
| Threading/JobSystem | ✅ 80% | Fiber-based job system, thread pool, wait-free queues |
| Vulkan Backend | ✅ 90% | Robust cross-platform Vulkan implementation |
| Water System | ✅ 70% | Wave simulation, GPU reflection/refraction, buoyancy |
| WebUI Editor | ✅ 80% | Browser-based editor with scene/game viewport, hierarchy, inspector |

**Overall: ~90%**

## Demo Projects

| Project | Description | Scripts | Status |
|---------|-------------|---------|--------|
| **MetroidvaniaDemo** | NES-style (256×224) 2D platformer with ability gating, 3 rooms, enemies, pickups, save points, HUD, particles, 2D lighting, audio zones | 16 C# scripts | ✅ Complete |
| **Prisma2D** | 2D rendering and lighting reference project | 5 C# scripts | ✅ Complete |
| **PathTracing3D** | Real-time path tracing tech demo | 2 C# scripts | ✅ Complete |
| **PrismaCraft** | Minecraft-style voxel world (in development) | 1 C# script | 🚧 WIP |
| **Deferred3D** | Deferred rendering pipeline demo | C++ only | ✅ Complete |
| **ClusteredForward3D** | Clustered forward rendering demo | C++ only | ✅ Complete |
| **PacManGame** | Classic Pac-Man arcade demo | C++ only | ✅ Complete |

## Quick Start

Prisma Engine uses **CMake FetchContent** for dependency management by default - no manual library installation required.

### Windows

```bash
# Clone repository
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# Build with CMake Presets
cmake --preset windows-x64-debug
cmake --build build/windows-x64-debug --parallel
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libx11-dev libxrandr-dev libvulkan-dev

# Build
cmake --preset linux-x64-debug
cmake --build build/linux-x64-debug --parallel
```

### Android

```bash
# Open in Android Studio
# Path: projects/android/PrismaAndroid
# Dependencies download automatically via CMake FetchContent
```

## Using the SDK (Without Full Source)

If you just want to try out a project (like PrismaCraft) or build your own game, you don't need the full engine source — download the **prebuilt SDK** from GitHub Releases:

### Option 1: Download Prebuilt SDK (Recommended)

```bash
# 1. Download from https://github.com/Excurs1ons/PrismaEngine/releases
#    Look for: PrismaEngine-SDK-<version>-<platform>.tar.gz

# 2. Extract
tar xzf PrismaEngine-SDK-0.1.0-linux.tar.gz

# 3. Build any sample project
cd PrismaEngine-SDK-0.1.0-linux/samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=../..
cmake --build build
./build/BasicTriangle
```

### Option 2: Use SDK from Local Repo

```bash
# SDK is already in the repo at sdk/
cd sdk/samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=$(pwd)/../..
cmake --build build
./build/BasicTriangle
```

The SDK contains:
- **190+ public C++ headers** — Full engine API (`#include <PrismaEngine/PrismaEngine.h>`)
- **CMake config** — `find_package(PrismaEngine)` ready
- **Sample projects** — BasicTriangle, BlockGame, PrismaCraftStarter
- **Precompiled libraries** — Engine binaries for your platform (in release downloads)

> **Note**: Prebuilt SDK is available on [GitHub Releases](https://github.com/Excurs1ons/PrismaEngine/releases). The `sdk/` directory in the repo contains headers and CMake configs but no prebuilt binaries — those are too large for the repository.

## Documentation

- [Documentation Index](docs/Index.md) - **Start here**
- [Architecture Overview](docs/README_zh.md) - Architecture and design
- [Vulkan Integration](docs/VulkanIntegration.md) - Detailed Android implementation
- [CoreCLR Scripting Summary](docs/plans/archived/2026-05-10-coreclr-scripting-summary.md) - C# scripting implementation
- [RenderGraph Plan](docs/RenderGraphMigrationPlan.md) - Future rendering roadmap
- [MetroidvaniaDemo Design](docs/plans/2026-05-26-metroidvania-demo-design.md) - 2D platformer architecture and design

## Core Features

- **Modern C++23**: Utilizing concepts, coroutines, and designated initializers.
- **Smart Dependency Management**: No manual library installation required; CMake handles everything.
- **Unified Rendering API**: Write once, run on DX12 or Vulkan.
- **Android Deep Optimization**: Zero-latency input via GameActivity and high-performance Vulkan rendering path.
- **CoreCLR Scripting**: Full C# scripting with SoA entity pool and self-contained deployment.
- **MCP Integration**: Full Model Context Protocol support for AI Agent control (17 tools, hash delta tracking, double transport).
- **WebUI Editor**: Browser-based full editor with real-time scene viewport, hierarchy, inspector and console.
- **2D Platformer Physics**: AABB platform collision resolver, sweep test, one-way platforms, quadtree spatial acceleration
- **NES-Style Retro Pipeline**: PixelPerfect upscaling + CRT scanline effects for retro pixel-art games
- **C# Game Logic Layer**: 16-script full game logic in C# via CoreCLR — player controller, AI, HUD, particles, audio zones

## TODO / Remaining Work

### Subsystems — Specific Gaps

| Subsystem | Current % | Gap |
|-----------|-----------|-----|
| Profiling | 60% | ImGui profiler panel not built; no custom GPU/CPU marker API |
| Testing | 65% | No 2D engine-specific tests (Physics2D, Pipeline2D, Tilemap, AudioZone) |
| Animation | 70% | SoA rendering pipeline not wired to `SpriteAnimationComponent` |
| Network | 70% | Missing encryption (TLS/DTLS), no client-side lag compensation/prediction |
| Water | 70% | No foam/whitecap simulation, no shallow-water caustics |
| Shaders | 78% | Missing HLSL post-processing variants (Bloom, Grayscale, Distortion) |
| Particle | 78% | No hybrid GPU+CPU particle rendering path |
| Editor | 80% | Undo/redo support for scene mutations not implemented |
| Console | 78% | No paginated scroll-back history |
| Terrain | 83% | LOD transition stiching/blending not implemented |
| AI | 85% | No BehaviorTree visual debugger (ImGui node graph) |
| Audio | 85% | BGM `.ogg` files not yet in `assets/audio/` |
| Threading | 85% | Fiber-based job system lacks cross-platform test coverage |
| Navigation | 93% | Dynamic obstacle NavMesh updates not implemented |
| 2D Physics | 98% | `SweepAABB` hit.normal edge cases on corner collisions |

### Known Non-Blocking Issues

- `SpriteAnimationComponent` render path not yet integrated into SoA drawing — falls back to manual UV update
- BGM audio files (`.ogg`) not present in `assets/audio/` — AudioZone system itself is complete
- No C# unit tests for ScriptEngine interop calls — engine-side interop is fully stubbed and implemented

## License

MIT License - see [LICENSE](LICENSE) for details.
