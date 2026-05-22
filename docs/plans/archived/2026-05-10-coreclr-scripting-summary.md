# CoreCLR C# Scripting — Implementation Summary

**Commit:** `75b29bb` (prune)
**Status:** ✅ Functional — Template2D fully scripted in C#

## Architecture

```
C++ Engine (PrismaEngine)
├── CoreCLRHost        hostfxr init → self-contained publish
├── ScriptEngine       PrismaAPI (18 fns) + entity pool + camera sync
├── Engine::Run()      Bootstrap() → OnFrame() per frame
└── Template2DApp      minimal: OnInitialize/OnRender/OnEvent

CoreCLR Runtime
└── hostfxr initialized from scripts/hostfxr.dll (self-contained)

C# PrismaEngine.Core.dll
├── Node          entity = position (no GameObject/Transform split)
├── Script        base class (OnCreate/OnStart/OnUpdate/OnDestroy)
├── Input         GetKey(KeyCode), MousePosition
├── Time          DeltaTime, Elapsed
└── Math types    Vector2, Color, Random, KeyCode (SDL3 scancodes)

C# GameScripts.dll
├── ScriptEntry.Bootstrap()   entry point
├── ScriptEntry.OnFrame()     per-frame callback
├── SceneInit                 20 sprites + camera
├── RotatingSprite            rotation animation
└── CameraController          WASD + arrow key movement
```

## Key Design Decisions

| Decision | Rationale |
|---|---|
| **Unity-style** (C# owns entities) | C# `Node` is the entity; C++ provides backend |
| **No GameObject/Transform split** | `Node.Position/Rotation/Scale` directly |
| **Not "MonoBehaviour"** | Base class is `Script` |
| **Self-contained publish** | No system .NET dependency at runtime |
| **hostfxr from local dir** | CoreCLRHost loads hostfxr from scripts/ dir |
| **Bootstrap callback** | C++ passes PrismaAPI function table at init |
| **OnFrame per frame** | C++ calls C#_OnFrame(dt) each frame |
| **SDL3 scancodes in KeyCode** | `W=26`, `Arrows=79-82`, `Escape=41` |

## Template2D Behaviors (C#)

- **5 static scene sprites** from `2d_test.jsonc` (rendered by C++)
- **20 dynamic rotating sprites** created by `SceneInit.cs`
- **Camera movement** via WASD/arrows in `CameraController.cs`
- **FPS/GPU/entity stats** rendered as HUD overlay

## File Map

| Layer | Files | Lines |
|---|---|---|
| C++ Engine (NEW) | `scripting/CoreCLRHost.h/.cpp` | ~180 |
| C++ Engine (NEW) | `scripting/ScriptEngine.h/.cpp` | ~240 |
| C++ Engine (MOD) | `app/Engine.h/.cpp` | +~40 |
| C++ Template2D (MOD) | `src/Template2DApp.cpp/.h` | ~190 |
| C++ Build (MOD) | `CMakeLists.txt` (2 files) | +~60 |
| C# Core | `PrismaEngine.Core/` (11 files) | ~400 |
| C# Game | `GameScripts/` (6 files) | ~150 |
| **Total** (new + modified) | **~27 files, ~1400 lines** | |

## Build Flow

```
cmake --build --preset editor-windows-x64-debug
  → dotnet publish --self-contained -r win-x64 (C# → DLLs + hostfxr)
  → MSVC compile (C++ → Template2D.exe)
  → copy scripts/ to output dir
```

## Running

```
Template2D.exe  (working dir = build/<preset>/bin/Debug/)
  → Engine::Run()
    → CoreCLRHost::Initialize("scripts/")
    → ScriptEntry.Bootstrap(api)
    → SceneInit creates 20 sprites
    → Loop: OnFrame(dt) → RenderFrame()
```

## Bugs Encountered & Fixes

| Bug | Root Cause | Fix |
|---|---|---|
| Black sprites | `Random.Range(0,1)` → int overload → always 0 | Use `(float)Range(0,100)/100f` |
| WASD not moving | KeyCode enum used ASCII (W=87) but events carry SDL scancode (W=26) | Fixed values to match IInputDriver.h/SDL3 |
| Input never reached pressedKeys | Template2DApp::OnEvent overrode base without calling Application::OnEvent | Added `Application::OnEvent(e)` call |
| hostfxr not found | Probing system paths (security/portability) | Self-contained publish, load from scripts/ only |
| Self-contained + library fail | `load_assembly_and_get_function_pointer` doesn't support self-contained components | `<OutputType>Exe</OutputType>` + empty Program.cs |
