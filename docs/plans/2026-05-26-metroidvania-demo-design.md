# Metroidvania Demo — Design Document

**Date:** 2026-05-26
**Project:** PrismaEngine — Retro pixel-art Metroidvania tech demo
**Branch:** `dev`

## Overview

Build a NES-style (256×224) pixel-art Metroidvania tech demo using PrismaEngine's latest technology (Vulkan, C++23, C# CoreCLR). The project prioritizes engine infrastructure improvements — all reusable components go into the C++ engine core with C# bindings, while game-specific logic lives in C# scripts.

## Architecture

```
┌───────────────────────────────────────────────────────┐
│                  C++ Engine Core                        │
│  (Reusable across all 2D projects)                      │
│                                                         │
│  Pipeline2D: Light2D → Opaque → Canvas                 │
│              → PixelPerfectPass → PostProcessPass2D → UI│
│  Tilemap:    Grid + Tileset + Collision + JSON          │
│  Physics2D:  Pure 2D AABB collision + platform          │
│  PixelCam:   Integer scale helper                       │
│  SRP fixes:  CreateRenderTarget + Render() call         │
└───────────────────────┬───────────────────────────────┘
                        │ PrismaAPI (C# bindings)
┌───────────────────────▼───────────────────────────────┐
│              C# Game Layer (MetroidvaniaDemo)          │
│                                                         │
│  PlayerController    Movement / jump / dash             │
│  CameraFollow        Follow player with pixel snap      │
│  AbilityGate         Lock/unlock rooms                  │
│  TilemapCollider     Read tile collision → Physics2D    │
└───────────────────────────────────────────────────────┘
```

## Engine Changes (C++)

### E1 — SRP: Connect ScriptEngine::Render()

- **File:** `src/engine/app/Engine.cpp`
- Add `m_scriptEngine->Render(dt)` call between `BeginFrame()` and `EndFrame()`, gated by `renderMode == SRP`
- Non-SRP modes unchanged

### E2 — SRP: CreateRenderTarget

- **File:** `src/engine/scripting/SRPGraphicsAPI.cpp`
- Implement `CreateRenderTarget()` — currently returns 0 (stub)
- Create `ITextureRenderTarget` proxy class (ref: `LightTextureRTProxy` in `Light2DPass.cpp`)
- Uses `IResourceFactory::CreateTextureImpl()` with `allowRenderTarget=true`

### E3 — SRP: CmdBeginRenderPass RT lookup

- **File:** `src/engine/scripting/SRPGraphicsAPI.cpp`
- Replace `// TODO: lookup RT handle` with actual handle resolution from `m_renderTargets`

### E4 — PixelPerfectPass (new)

- **File:** `src/engine/graphic/2d/PixelPerfectPass.h/.cpp`
- Creates 256×224 offscreen `ITextureRenderTarget`
- After OpaquePass + CanvasPass render to offscreen RT
- Blits to swap chain with integer nearest-neighbor scaling
- Calculates integer scale factor from window size / 256×224
- Centers output with black letterbox bars if aspect ratio mismatches
- **Pipeline2D order:** `Light2D → Opaque → Canvas → PixelPerfect → PostProcess → UI`

### E5 — PostProcessPass2D (CRT)

- **File:** `src/engine/graphic/2d/PostProcessPass2D.cpp`
- Implement actual CRT scanline + chromatic aberration shader
- Wire up `Process()` with command buffer draw calls (currently commented out)
- Toggle via `SetEffect(EffectType::CRT, true/false)`

### TT1 — Tilemap System

New module at `src/engine/tilemap/`:

```
src/engine/tilemap/
├── Tilemap.h / .cpp         # Grid container + JSON serialization
├── TileSet.h / .cpp         # Tile definitions + spritesheet reference
├── TileLayer.h / .cpp       # 2D grid layer
└── TilemapRenderer.h / .cpp # Batch rendering + frustum culling
```

**Data structures:**
```cpp
struct TileDef {
    uint32_t tileId;
    uint32_t texIndex;    // Index into tileset spritesheet
    uint8_t  collision;   // Bitmask: Solid(1), Platform(2), Hazard(4), Ladder(8)
};

struct TileLayer {
    std::string name;
    uint32_t width, height;
    std::vector<uint32_t> tiles;  // 0 = empty
};

class Tilemap {
    Ref<TileSet> m_tileSet;
    std::vector<TileLayer> m_layers;
    uint32_t m_tileSize;          // Typically 16px
    
    bool LoadFromJSON(std::string_view path);
    bool SaveToJSON(std::string_view path);
};
```

**Rendering:** TilemapRenderer iterates visible tile range and issues batched `DrawQuad` calls via `Renderer2D`.

**C# binding:** Exposed through PrismaAPI function pointers:
```csharp
class Tilemap {
    static Tilemap Load(string path);
    int GetTile(int layer, int x, int y);
    bool IsSolid(int x, int y);
    int Width { get; }
    int Height { get; }
}
```

### PP1 — Physics2D System

New module at `src/engine/physics2d/` — pure 2D, no 3D AABB wrapping:

```
src/engine/physics2d/
├── AABB2D.h              # 2D bounding box (minX/Y, maxX/Y)
├── Physics2D.h / .cpp    # Collision detection + queries
```

**Pure 2D implementation** — no Z-axis overhead, float-precision:

```cpp
struct AABB2D {
    float minX, minY, maxX, maxY;
    bool intersects(const AABB2D& o) const;
};

class Physics2D {
    static bool checkAABB(const AABB2D& a, const AABB2D& b);
    static bool sweepAABB(const AABB2D& moving, glm::vec2 velocity,
                          const AABB2D& static_, float& hitTime, glm::vec2& normal);
    static bool resolvePlatform(const AABB2D& player, glm::vec2& velocity,
                                const AABB2D* solids, uint32_t count,
                                bool& onGround, bool& hitCeiling);
    static bool checkOneWayPlatform(const AABB2D& player,
                                    const AABB2D& platform, bool wasAbove);
    
    struct RaycastHit2D { ... };
    static RaycastHit2D rayCast(glm::vec2 origin, glm::vec2 direction,
                                float maxDist, const AABB2D* targets, uint32_t count);
};
```

**C# binding:**
```csharp
struct AABB2D { float MinX, MinY, MaxX, MaxY; }
static class Physics2D {
    static bool CheckAABB(AABB2D a, AABB2D b);
    static bool ResolvePlatform(AABB2D player, ref Vector2 velocity,
        AABB2D[] solids, out bool onGround, ...);
}
```

### Pipeline2D Update

**File:** `src/engine/graphic/2d/Pipeline2D.h/.cpp`

Execution order becomes:
```
1. Light2DPass      → Offscreen light texture
2. OpaquePass       → Offscreen pixel RT (256×224)
3. CanvasPass2D     → Offscreen pixel RT (overlay)
4. PixelPerfectPass → Offscreen → Swap chain (integer scale)
5. PostProcessPass2D→ Swap chain (CRT effect)
6. UIPass2D         → Swap chain (screen-space UI)
```

Project `MetroidvaniaDemo.json` enables pixel-perfect mode with:
```json
{
    "renderMode": "2D",
    "pixelPerfect": true,
    "crtEffect": true,
    "pixelWidth": 256,
    "pixelHeight": 224
}
```

## Game Project: MetroidvaniaDemo

### File Structure

```
projects/MetroidvaniaDemo/
├── CMakeLists.txt
├── assets/
│   ├── MetroidvaniaDemo.json     # Project config (1024×896 window = 256×224 ×4)
│   ├── textures/
│   │   ├── dungeon_tileset.png   # 16×16 tile spritesheet
│   │   └── player.png            # Player sprite
│   └── maps/
│       └── test_dungeon.json     # Tilemap (3 rooms)
├── src/
│   ├── main.cpp
│   ├── MetroidvaniaApp.h / .cpp
│   └── CreateApplication.cpp
└── scripts/
    ├── GameScripts.csproj
    └── GameScripts/
        ├── ScriptEntry.cs
        ├── PlayerController.cs
        ├── CameraFollow.cs
        ├── AbilityGate.cs
        ├── DashPickup.cs
        └── Enemy.cs
```

### Demo Scope: 3 Rooms

```
┌──────────┐     ┌──────────┐     ┌──────────┐
│   Room 1  │────│  Room 2   │────│  Room 3   │
│  (Start)  │    │  (Mid)    │    │  (End)    │
│           │    │          │    │          │
│  Spawn →  │    │ Dash gate │    │  Chest    │
│  No enemies│   │ 1 enemy  │    │  (goal)   │
│  Tutorial │    │ Dash pickup│   │           │
└──────────┘    └──────────┘    └──────────┘
```

**Player abilities:** Movement → Jump → Dash (unlocked in Room 2)
**Ability gate:** Gap too wide for jump, requires dash
**Tech validation points:**
- 256×224 → 1024×896 integer scaling (4×)
- Tilemap loading, rendering, collision
- Physics2D platform collision + one-way platforms
- C# game logic (player controller, camera, abilities)
- Room transitions
- CRT post-processing toggle

### C# Scripts

| Script | Responsibility |
|--------|---------------|
| `ScriptEntry.cs` | Bootstrap + per-frame update |
| `PlayerController.cs` | Movement, jump, dash, physics interaction |
| `CameraFollow.cs` | Smooth/tight camera follow with pixel snap |
| `AbilityGate.cs` | Room transition trigger with ability check |
| `DashPickup.cs` | Collectable that unlocks dash |
| `Enemy.cs` | Simple patrol enemy with damage |

## CMake Integration

- New option `PRISMA_BUILD_PROJECT_METROIDVANIADEMO` in `projects/CMakeLists.txt`
- Follows same pattern as `PRISMA_BUILD_PROJECT_PRISMA2D`
- Links against `Engine` library
- Builds C# project as `MetroidvaniaDemo_Managed.dll`

## Key Decisions

1. **Rendering in C++ (not C# SRP):** Pixel-perfect and CRT are engine-level features, toggleable via project config. Keeps C# focused on game logic.
2. **Pure 2D Physics (not 3D wrapper):** Custom 2D AABB avoids Z-axis overhead, simpler code, better cache performance.
3. **Tilemap in C++:** JSON-serialized, batch-rendered, collision data accessible from both C++ and C#.
4. **SRP fixes still applied:** Even though this game doesn't use SRP, fixing `CreateRenderTarget` and `Render()` wiring makes the SRP system viable for future projects.
5. **Demo scale:** 3 rooms, 1 ability, 1 enemy type — validates all tech without building a full game.

## Timeline Estimate

| Module | Est. Effort |
|--------|-------------|
| SRP fixes (E1-E3) | Small |
| PixelPerfectPass (E4) | Medium |
| PostProcessPass2D CRT (E5) | Small |
| Tilemap system (TT1) | Medium |
| Physics2D (PP1) | Medium |
| Project scaffolding | Small |
| C# game scripts | Medium |
| Pixel art assets | Small |
| **Total** | **Medium-large** |
