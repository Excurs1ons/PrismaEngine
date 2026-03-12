# Prisma Engine - Comprehensive Code Review
## The Cherno & Linus Torvalds Perspective

**Review Date**: 2026-03-09
**Reviewers**: The Cherno (architecture-focused), Linus Torvalds (pragmatic)
**Status**: Findings Complete | Refactoring In Progress

---

## Executive Summary

Prisma Engine is a modern C++20 cross-platform game engine with a solid foundation but suffers from **hybrid architecture confusion**. The codebase mixes ECS (Entity Component System) with traditional OOP (Object-Oriented Programming), resulting in significant architectural debt.

**Key Findings**:
- ✅ **Good**: Rendering abstraction with clean interfaces
- ✅ **Good**: ECS core with compile-time type IDs
- ❌ **Critical**: Components have virtual functions (anti-pattern)
- ❌ **Critical**: Hybrid GameObject + Entity architecture
- ❌ **Critical**: Audio system has dual XAudio2 implementations (stub vs working)
- ❌ **High**: Asset loaders are stubs (MeshAsset loads hardcoded triangle)
- ❌ **High**: Heavy singleton usage (8+ global instances)
- ❌ **High**: No async loading support (AsyncLoader is stub)

---

## Severity Assessment

| Category | Issues Found | Critical | High | Medium | Low |
|-----------|--------------|----------|-------|-----|
| **ECS Architecture** | 4 | 2 | 2 | 3 |
| **Audio System** | 2 | 3 | 2 | 1 |
| **Asset Management** | 1 | 4 | 3 | 2 |
| **Rendering System** | 1 | 2 | 2 | 4 |
| **Platform Abstraction** | 0 | 1 | 2 | 2 |
| **Overall** | **8** | **12** | **11** | **12** |

---

## Section 1: ECS & Component System Analysis

### Critical Issues

#### 1.1 Components Use Virtual Functions (CRITICAL)
**File**: `src/engine/Component.h`

```cpp
class Component {
public:
    virtual ~Component() = default;
    virtual void Initialize(){};
    virtual void Update([[maybe_unused]] float deltaTime) {}
    virtual void Shutdown(){};
    GameObject* owner = nullptr;
};
```

**Problem**: Components should be POD (Plain Old Data) structures for cache efficiency. Virtual functions add:
- VTable pointer overhead (8 bytes per component)
- Cache misses due to indirection
- OOP mixing with ECS (anti-pattern)

**The Cherno's View**: Components should be pure data structures with no methods
**Linus's View**: Unnecessary complexity - "just store the damn data"

**Impact**: All components that inherit from `Component` (Transform, Camera, MeshRenderer, etc.) carry this overhead

---

#### 1.2 Component Owner Pointer (CRITICAL)
**File**: `src/engine/Component.h:23`

```cpp
GameObject* owner = nullptr;
```

**Problem**: In proper ECS, components should NOT know about their entities. This creates:
- Circular dependencies
- Tight coupling between components and game objects
- Makes component iteration inefficient

**The Cherno's View**: Components should be accessed via EntityID, not through GameObject references
**Linus's View**: Hidden dependencies make testing impossible

**Impact**: Every component has an 8-byte pointer overhead and can't be moved in memory efficiently

---

#### 1.3 Hybrid GameObject + Entity Architecture (CRITICAL)
**Files**:
- `src/engine/GameObject.h` - OOP GameObject with component list
- `src/engine/Scene.h` - Uses GameObjects
- `src/engine/core/ECS.h` - Entity + World + Component pools

**Problem**: TWO parallel systems exist:
1. **GameObject** system: Traditional OOP with `std::shared_ptr<Transform>`, `std::vector<std::shared_ptr<Component>>`
2. **Entity** system: ECS with `ComponentPool<T>`, efficient iteration

**GameObject issues**:
- Uses `std::dynamic_pointer_cast` for component lookup (line 43) - **SLOW**
- Stores Transform separately (not as component)
- Has its own component iteration loop (lines 29-32)

**ECS issues**:
- Entity class is just a wrapper around EntityID
- World singleton manages everything
- ComponentPool stores components in vectors (cache-friendly)

**The Cherno's View**: Choose ONE paradigm - either pure ECS or pure OOP, not both
**Linus's View**: "What the hell is this? Two systems doing the same thing?"

**Impact**: Architecture confusion makes code hard to understand and maintain

---

#### 1.4 Transform Hierarchy Not in Components (HIGH)
**Files**:
- `src/engine/TransformSystem.h:35` - References non-existent `TransformComponent`
- `src/engine/Scene.h` - Uses GameObject Transform
- `src/engine/GameObject.h:70` - Has separate Transform fields

**Problem**: Transform is stored:
- As a separate `GameObject` member
- NOT as an ECS component
- TransformSystem has its own hierarchy map (not component-based)

**Impact**: Can't use ECS iteration for transforms, mixing paradigms

---

#### 1.5 Components Have Non-POD Members (HIGH)
**Files**: Multiple component files
- `src/engine/graphic/RenderComponent.h` - Has `std::shared_ptr<Material>`
- `src/engine/graphic/MeshRenderer.h` - Has `std::vector<Submesh>`

**Problem**: Non-POD members cause:
- Cache misses (pointer indirection)
- Memory fragmentation
- Inefficient iteration

**Impact**: Defeats the purpose of ECS (cache-friendly data access)

---

#### 1.6 Duplicate ScriptComponent Definitions (MEDIUM)
**Files**:
- `src/engine/ScriptComponent.h` - Empty class inheriting from Component
- `src/engine/scripting/ScriptSystem.h:8-32` - Proper struct with data

```cpp
// src/engine/ScriptComponent.h - EMPTY!
class ScriptComponent : public Component {
};

// src/engine/scripting/ScriptSystem.h:8-32 - HAS DATA!
struct ScriptComponent {
    std::string className;
    std::string assemblyName;
    EntityID entity;
};
```

**Problem**: Two conflicting definitions. Which one is used?

**Impact**: Compilation errors or confusion

---

### High-Priority Issues

#### 1.7 Unsafe Iteration Pattern (HIGH)
**File**: `src/engine/scripting/ScriptSystem.cpp:33-45`

```cpp
auto& components = pool->GetData();
for (auto& script : components) {
    // Iterates over ALL components, not just alive entities!
}
```

**Problem**: Iterates over entire component pool without checking if entity is alive

**The Cherno's View**: Should iterate over World::GetAliveEntities()
**Linus's View**: "This is wrong on multiple levels"

**Impact**: Processes deleted components (bugs) + wasted CPU cycles

---

#### 1.8 No Component Views/Queries (HIGH)
**Problem**: No efficient way to query entities with specific component sets
- Systems iterate over all components manually
- No "View<Comp1, Comp2, Comp3>" pattern
- No archetype filtering

**The Cherno's View**: Use component views for cache-friendly iteration
**Impact**: O(N×M) complexity instead of O(N) for multi-component queries

---

### Medium-Priority Issues

#### 1.9 Multiple Inheritance (MEDIUM)
**File**: `src/engine/graphic/Camera.h:9`

```cpp
class Camera : public Component, public ICamera {
```

**Problem**: Multiple inheritance adds complexity and ambiguity
- VTable for Component
- VTable for ICamera
- Which Update() gets called?

**Linus's View**: "Multiple inheritance is a disaster waiting to happen"

---

#### 1.10 ECS Entity is Just Wrapper (MEDIUM)
**File**: `src/engine/core/ECS.h:309-342`

```cpp
class Entity {
    EntityID m_id;
    World* m_world;  // Just wrapper around EntityID!
};
```

**Problem**: Extra indirection for simple type
- Could just use `EntityID` directly
- No real value added

**Linus's View**: "Unnecessary wrapper class"

---

## Section 2: Audio System Analysis

### Critical Issues

#### 2.1 Dual XAudio2 Implementations (CRITICAL)
**Files**:
- `src/engine/audio/AudioDeviceXAudio2.cpp` - **STUB** implementation
- `src/engine/audio/drivers/AudioDriverXAudio2.cpp` - **WORKING** driver

**AudioDeviceXAudio2.cpp issues**:
```cpp
bool AudioDeviceXAudio2::Initialize(const AudioDesc& desc) {
    m_desc = desc;
    m_initialized = true;  // JUST SETS FLAG!
    LOG_INFO("Audio", "XAudio2 device initialized (minimal implementation)");
    return true;
}
```

**AudioDriverXAudio2.cpp** (in `drivers/` folder) - Has actual XAudio2 API calls, buffer submission, playback logic

**Problem**: System has TWO paths:
1. Direct IAudioDevice implementation (stubbed out)
2. High-level AudioDevice using IAudioDriver (working)

**Which one is used?**: AudioDevice.cpp uses IAudioDriver via AudioDevice class

**Result**: AudioDeviceXAudio2 is DEAD CODE - never executed!

**Linus's View**: "Why is this here? Delete it or finish it!"

**Impact**: Confusing architecture, misleading code, wasted maintenance effort

---

#### 2.2 Memory Leak in XAudio2 Callback (CRITICAL)
**File**: `src/engine/audio/drivers/AudioDriverXAudio2.cpp:173`

```cpp
new XAudio2Callback(this, sourceId)  // Created with NEW
// ... never deleted!
```

**The Cherno's View**: Use RAII wrapper classes
**Linus's View**: "Memory leak - you're doing it wrong"

**Impact**: Every audio source created/destroyed leaks memory

---

### High-Priority Issues

#### 2.3 SDL3 Features Are Stubs (HIGH)
**File**: `src/engine/audio/AudioDeviceSDL3.cpp`

```cpp
void AudioDeviceSDL3::SetPitch(AudioVoiceId voiceId, float pitch) {
    m_voices[voiceId].pitch = pitch;  // Stored but NOT APPLIED!
}

void AudioDeviceSDL3::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    // Lines 346-357 - Just comments, no implementation!
}
```

**Problem**: SDL3 has working playback but:
- No 3D panning (stereo mixing)
- No pitch shifting (requires resampling)
- Distance attenuation is only volume-based

**Impact**: Feature-complete appearance but not actually working

---

#### 2.4 Working Backends Disabled (HIGH)
**File**: `src/engine/config/AudioBackendConfig.h`

```cpp
#define PRISMA_ENABLE_AUDIO_XAUDIO2  1  // Enabled on Windows only
#define PRISMA_ENABLE_AUDIO_OPENAL    0  // Disabled!
#define PRISMA_ENABLE_AUDIO_SDL3     0  // But code exists and works!
```

**Problem**: SDL3 and OpenAL have working implementations but are disabled
- Only XAudio2 (Windows) is enabled
- Android uses AAudio (good)
- Linux has nothing

**Impact**: Cross-platform audio broken

---

### Medium-Priority Issues

#### 2.5 No RAII for Audio Resources (MEDIUM)
**Files**: Multiple audio files

**Problem**: Manual new/delete for:
- XAudio2 voices
- SDL streams
- OpenAL buffers

**The Cherno's View**: Use smart pointers or wrapper classes

**Impact**: Exception-safety violations, potential memory leaks

---

#### 2.6 Audio Queue is Software-Based (MEDIUM)
**File**: `src/engine/audio/AudioDeviceSDL3.cpp:614-676`

**Problem**: Mixing happens in software into fixed buffer
- No async streaming
- All audio loaded into memory upfront
- No proper queue management

**Impact**: Memory intensive for large audio files

---

## Section 3: Asset Management Analysis

### Critical Issues

#### 3.1 MeshAsset Loads Hardcoded Triangle (CRITICAL)
**File**: `src/engine/resource/MeshAsset.cpp:13-43`

```cpp
bool MeshAsset::Load(const std::filesystem::path& path) {
    // ... ignores path parameter ...

    // HARDCODED TRIANGLE!
    positions = {
        { -0.5f, -0.5f, 0.0f },
        {  0.5f,  -0.5f, 0.0f },
        {  0.0f,   0.5f, 0.0f }
    };

    indices = { 0, 1, 2 };

    LOG_WARNING("Mesh", "Loaded hardcoded triangle instead of file: {0}", path.string());
    return true;
}
```

**Problem**: Mesh loader completely ignores file parameter, always returns triangle

**The Cherno's View**: This is not an asset loader - it's a stub
**Linus's View**: "This is bullshit - delete it"

**Impact**: Can't actually load any mesh data

---

### High-Priority Issues

#### 3.2 No Async Loading Support (HIGH)
**File**: `src/engine/core/AsyncLoader.cpp:129-154`

```cpp
uint64_t loadTexture(...) override { return 0; }  // STUB
uint64_t loadModel(...) override { return 0; }    // STUB
LoadingProgress getProgress() const override { return {}; }
bool isLoading() const override { return false; }
```

**Problem**: AsyncLoader exists but ALL methods are stubs!
- AssetManager::Load<T>() is synchronous
- No callback mechanism for completion
- No progress tracking

**The Cherno's View**: Async loading is essential for game engines
**Linus's View**: "Why is this code here? It does nothing"

**Impact**: Assets block main thread, poor user experience

---

#### 3.3 TextureAsset Stub for Non-Android (HIGH)
**File**: `src/engine/TextureAsset.cpp:235-243`

```cpp
#ifdef PRISMA_ENABLE_RENDER_VULKAN
    // ... working Vulkan implementation ...
#else
    LOG_ERROR("Texture", "非 Android 平台实现");  // "Not implemented for non-Android platform"
    return false;
#endif
```

**Problem**: TextureAsset only works on Android/Vulkan, returns error on Windows/Linux

**Impact**: Cross-platform texture loading broken on desktop

---

#### 3.4 No Hot-Reload Support (HIGH)
**File**: `src/engine/ResourceManager.cpp:369-378`

```cpp
void ResourceManager::CheckFileModifications() {
    for (auto it = m_fileTimestamps.begin(); it != m_fileTimestamps.end(); ++it) {
        if (std::filesystem::exists(it->first)) {
            auto currentTime = std::filesystem::last_write_time(it->first);
            if (currentTime > it->second) {
                it->second = currentTime;  // ONLY updates timestamp!
                // NO RELOAD!
            }
        }
    }
}
```

**Problem**: CheckFileModifications() updates timestamps but does NOT reload assets
- No callback to trigger reload
- No system to update loaded assets

**The Cherno's View**: Hot-reload is critical for iteration speed
**Impact**: Can't iterate on assets without restarting engine

---

### Medium-Priority Issues

#### 3.5 Incomplete Serialization (MEDIUM)
**File**: `src/engine/tilemap/TilemapAsset.cpp:71,104`

```cpp
// TODO: 序列化完整地图数据  // "Serialize complete map data"
// TODO: 反序列化完整地图数据  // "Deserialize complete map data"
```

**Problem**: Tilemap serialization is incomplete
- Only metadata serialized
- No vertex data serialization

**Impact**: Can't save/load tilemaps properly

---

#### 3.6 Material Load Does Nothing (MEDIUM)
**File**: `src/engine/graphic/Material.cpp:120-130`

```cpp
bool Material::Load(const std::filesystem::path& path) {
    // ... does nothing ...
    m_isLoaded = true;  // Just sets flag!
    return true;
}
```

**Problem**: Load method doesn't load anything
- No shader loading
- No texture loading
- Just marks as loaded

**Impact**: Materials are empty placeholders

---

#### 3.7 No Asset Dependency Tracking (MEDIUM)
**Problem**: Assets reference each other (e.g., Material → Shader, Material → Texture) but:
- No `std::weak_ptr` usage
- No dependency graph
- No circular reference detection

**The Cherno's View**: Dependency tracking prevents memory leaks
**Impact**: Potential cycles keeping assets alive forever

---

#### 3.8 No Fallback Handling (MEDIUM)
**File**: `src/engine/resource/ResourceFallbackImpl.cpp:12,42`

```cpp
std::shared_ptr<Mesh> ResourceFallback::CreateDefaultMesh() {
    return nullptr;  // Returns nullptr!
}

std::shared_ptr<Material> ResourceFallback::CreateDefaultMaterial() {
    return nullptr;
}
```

**Problem**: Fallback returns nullptr instead of creating default assets
- No error recovery
- No graceful degradation

**Impact**: Asset load failures crash or break rendering

---

## Section 4: Rendering System Analysis

### Critical Issues

#### 4.1 RenderGraph is Empty (CRITICAL)
**File**: `src/engine/graphic/RenderGraph.h`

```cpp
#pragma once
namespace PrismaEngine::Graphic {

class RenderGraph {
    // File is EMPTY! Only 7 lines!
};

}  // namespace
```

**Problem**: RenderGraph referenced in codebase but not implemented
- No render passes
- No dependency tracking
- No frame graph execution

**The Cherno's View**: Render graph is essential for modern engines
**Linus's View**: "Delete this file or implement it"

**Impact**: No render pass optimization, all draws are immediate mode

---

### High-Priority Issues

#### 4.2 DirectX 12 Adapter Missing (HIGH)
**File**: Not found in codebase

**Problem**: Vulkan adapter exists (`adapters/vulkan/`) but DX12 adapter is missing
- README says "DirectX 12 backend is in advanced development"
- CMake references DX12 but no implementation found

**Impact**: Windows rendering is Vulkan-only (not native DX12)

---

#### 4.3 No Proper RAII for GPU Resources (HIGH)
**Files**: Multiple rendering files

**Problem**: Manual new/delete for:
- Vulkan buffers
- Vulkan textures
- Vulkan fences

**The Cherno's View**: RAII wrappers prevent leaks
**Linus's View**: "Manual new/delete is asking for bugs"

**Impact**: Exception-safety violations, potential GPU memory leaks

---

### Medium-Priority Issues

#### 4.4 Upscaler Implementations Are Stubs (MEDIUM)
**Files**:
- `src/engine/graphic/upscaler/adapters/FSR/UpscalerFSR.cpp`
- `src/engine/graphic/upscaler/adapters/DLSS/UpscalerDLSS.cpp`
- `src/engine/graphic/upscaler/adapters/TSR/UpscalerTSR.cpp`

**Problem**: All upscalers have extensive TODO comments
- FSR: "TODO: 调用 FSR 3.1 API 进行超分辨率"
- DLSS: "TODO: 调用 DLSS API 进行超分辨率"
- TSR: "TODO: 执行 TSR 超分辨率"

**Impact**: Upscaler system exists but does nothing

---

## Section 5: Platform Abstraction Analysis

### Medium-Priority Issues

#### 5.1 Direct Windows.h Include (MEDIUM)
**File**: `src/engine/Platform.h`

```cpp
#if defined(_WIN32)
#include <Windows.h>
#endif
```

**Problem**: Platform.h has direct Windows.h include
- Not using IPlatform interface
- Platform-specific headers leak into core code

**Linus's View**: "This defeats the purpose of abstraction"

**Impact**: Core engine files include platform-specific headers

---

#### 5.2 Multiple Backup Files (MEDIUM)
**Files**: Multiple `.bak` files in audio/
- `AudioSystem.cpp.bak`
- `AudioBackend.h.bak`
- `AudioBackendSDL3.cpp.bak`
- etc.

**Problem**: Previous refactoring attempts left as backups
- Confusion about what's current
- Git should handle history

**Linus's View**: "Why are these .bak files in the repo?"

**Impact**: Codebase clutter, confusion about current state

---

## Section 6: Singleton & Global State Issues

### High-Priority Issues

#### 6.1 Excessive Singleton Usage (HIGH)
**Found 8 Singletons**:
1. Logger - Used everywhere
2. EngineCore - SharedSingleton
3. ThreadManager - Singleton
4. PhysicsSystem - Singleton
5. InputManager - Singleton
6. JobSystem - Singleton
7. SceneManager - Singleton
8. World (ECS) - Singleton

**Files**:
- `src/engine/Engine.cpp:17` - `static std::shared_ptr<EngineCore> instance`
- `src/engine/Logger.h:31` - `static Logger& GetInstance()`
- `src/engine/Singleton.h:12` - Template singleton base

**Linus's View**: "Global state is the enemy of modularity"

**Impact**:
- Hidden dependencies
- Difficult to test
- Can't have multiple instances

**The Cherno's View**: Use dependency injection instead

---

## Section 7: Placeholder/Incomplete Implementations

### High-Priority Issues

#### 7.1 ScriptSystem is Incomplete (HIGH)
**File**: `src/engine/scripting/ScriptSystem.cpp`

**Problem**: Mono integration referenced but not implemented
- Empty Awake/Start/Update methods (lines 123-134)
- No hot-reload
- No assembly loading

**Impact**: Scripting system exists but can't run scripts

---

#### 7.2 VoxelRenderer Incomplete (HIGH)
**File**: `src/engine/graphic/VoxelRenderer.cpp`

```cpp
// TODO: 设置材质并调用渲染命令  // "Set material and call render command"
// TODO: 实现贪婪网格生成算法  // "Implement greedy meshing algorithm"
```

**Problem**: Voxel rendering partially implemented
- No actual mesh generation
- No material rendering

**Impact**: Voxel system doesn't render anything

---

#### 7.3 UI System Minimal (MEDIUM)
**Files**: `src/engine/ui/`

**Problem**: UI system has basic components but:
- No layout system
- No event system
- No proper input handling

**Impact**: UI is not usable for editor or game

---

#### 7.4 Physics System Stub (MEDIUM)
**File**: `src/engine/PhysicsSystem.cpp`

```cpp
int PhysicsSystem::Initialize() {
    LOG_INFO("Physics", "Physics system initialized");
    return 0;  // Does NOTHING!
}

void PhysicsSystem::Update(float deltaTime) {
    (void)deltaTime;  // Empty implementation!
}
```

**Problem**: Physics system exists but does nothing
- No collision detection
- No rigid body simulation
- No integration with ECS

**Impact**: No physics simulation

---

## Prioritized Refactoring Plan

### Phase 1: Critical Fixes (DO FIRST)

#### P1.1 Fix MeshAsset::Load() to Load Actual Files
**File**: `src/engine/resource/MeshAsset.cpp:13-43`
**Action**: Implement proper mesh file loading
- Support .obj, .gltf, .fbx formats
- Parse vertex data, indices, submeshes
- Remove hardcoded triangle

**Effort**: 2-4 hours
**Priority**: CRITICAL

---

#### P1.2 Implement TextureAsset for Non-Android Platforms
**File**: `src/engine/TextureAsset.cpp:235-243`
**Action**: Implement OpenGL/DX12 texture loading
- Use STB for image loading
- Create GPU resources
- Handle mipmaps, compression

**Effort**: 2-3 hours
**Priority**: CRITICAL

---

#### P1.3 Remove AudioDeviceXAudio2 Stub
**File**: `src/engine/audio/AudioDeviceXAudio2.h` & `.cpp`
**Action**: Delete the stub implementation
- Keep AudioDriverXAudio2 (working version)
- Update AudioDevice to use driver directly

**Effort**: 1 hour
**Priority**: CRITICAL

---

#### P1.4 Fix XAudio2 Memory Leak
**File**: `src/engine/audio/drivers/AudioDriverXAudio2.cpp:173`
**Action**: Delete callback in destructor or use unique_ptr
```cpp
~AudioDriverXAudio2() {
    for (auto& source : m_sources) {
        if (source.callback) {
            delete source.callback;  // FIX THE LEAK
        }
    }
}
```

**Effort**: 30 minutes
**Priority**: CRITICAL

---

### Phase 2: High-Priority Refactoring

#### P2.1 Enable SDL3 Audio Backend
**File**: `src/engine/config/AudioBackendConfig.h:12`
**Action**: Change `PRISMA_ENABLE_AUDIO_SDL3` to 1 on non-Windows platforms
- SDL3 has working implementation
- Enable for Linux desktop

**Effort**: 30 minutes
**Priority**: HIGH

---

#### P2.2 Implement Async Loading
**File**: `src/engine/core/AsyncLoader.cpp:129-154`
**Action**: Implement async asset loading
- Return async task handles
- Call callbacks on completion
- Update progress

**Effort**: 4-6 hours
**Priority**: HIGH

---

#### P2.3 Implement Hot-Reload
**File**: `src/engine/ResourceManager.cpp:369-378`
**Action**: Implement actual asset reloading
- When file changes, unload and reload asset
- Notify dependent systems

**Effort**: 3-5 hours
**Priority**: HIGH

---

#### P2.4 Remove Virtual Functions from Component
**File**: `src/engine/Component.h:11-16`
**Action**: Remove virtual methods from Component base class
- Convert to POD struct
- Remove owner pointer
- Use ComponentID for entity association

**EFFORT WARNING**: This is a MAJOR refactoring that touches many files
**Better approach**: Create new POD component system alongside existing one
**Effort**: 8-12 hours
**Priority**: HIGH

---

#### P2.5 Resolve GameObject vs Entity Confusion
**Files**: `src/engine/GameObject.h`, `src/engine/Scene.h`
**Action**: Choose ONE paradigm and document it
- Option A: Pure ECS (delete GameObject, use Entity)
- Option B: Pure OOP (delete Entity/ComponentPool, use GameObject)

**Recommendation**: **Option A (Pure ECS)**
- Modern engines use ECS
- Better cache performance
- Aligns with core/ECS.h

**Effort**: 4-6 hours (decision + migration)
**Priority**: HIGH

---

#### P2.6 Fix ScriptSystem Iteration Bug
**File**: `src/engine/scripting/ScriptSystem.cpp:33-45`
**Action**: Only iterate over alive entities
```cpp
for (auto entity : world.GetAliveEntities()) {
    if (auto script = entity.GetComponent<ScriptComponent>()) {
        // Process only alive entities
    }
}
```

**Effort**: 30 minutes
**Priority**: HIGH

---

### Phase 3: Medium-Priority Improvements

#### P3.1 Implement RenderGraph
**File**: `src/engine/graphic/RenderGraph.h`
**Action**: Implement render pass graph
- Track render pass dependencies
- Optimize draw call sorting
- Enable render pass reuse

**Effort**: 6-8 hours
**Priority**: MEDIUM

---

#### P3.2 Complete SDL3 3D Audio
**File**: `src/engine/audio/AudioDeviceSDL3.cpp:346-357`
**Action**: Implement stereo panning for 3D audio
- Add proper HRTF filtering
- Implement pitch shifting with resampling

**Effort**: 2-3 hours
**Priority**: MEDIUM

---

#### P3.3 Add Component Views
**File**: `src/engine/core/ECS.h`
**Action**: Add View<T1, T2, T3> template for efficient iteration
- Cache-friendly multi-component queries
- Avoid iterating over all components

**Effort**: 4-6 hours
**Priority**: MEDIUM

---

#### P3.4 Implement Asset Serialization
**Files**: `src/engine/tilemap/TilemapAsset.cpp`, `src/engine/resource/MeshAsset.cpp`
**Action**: Complete serialization for all asset types
- Vertex data, indices, metadata
- Use existing JSON/binary serializers

**Effort**: 3-4 hours
**Priority**: MEDIUM

---

#### P3.5 Reduce Singleton Usage
**Files**: Multiple (8+ files)
**Action**: Replace singletons with dependency injection
- Pass dependencies through constructors
- Make systems non-singleton
- Allow multiple instances

**Effort**: 6-10 hours
**Priority**: MEDIUM

---

#### P3.6 Complete Material Loading
**File**: `src/engine/graphic/Material.cpp:120-130`
**Action**: Implement actual material file loading
- Parse material JSON/files
- Load shaders and textures
- Handle material properties

**Effort**: 2-3 hours
**Priority**: MEDIUM

---

#### P3.7 Implement Upscalers
**Files**: Upscaler adapter files
**Action**: Complete upscaler implementations
- Integrate FSR 3.1 SDK
- Integrate DLSS SDK
- Implement TSR

**Effort**: 8-12 hours per upscaler
**Priority**: MEDIUM

---

#### P3.8 Complete VoxelRenderer
**File**: `src/engine/graphic/VoxelRenderer.cpp`
**Action**: Implement greedy meshing algorithm
- Generate optimized geometry
- Render voxels properly

**Effort**: 6-10 hours
**Priority**: MEDIUM

---

### Phase 4: Low-Priority Polish

#### P4.1 Add RAII Wrappers
**Files**: Multiple rendering/audio files
**Action**: Create RAII wrapper classes for:
- Vulkan resources (buffers, textures, images)
- Audio resources (voices, buffers)
- Platform resources (threads, mutexes)

**Effort**: 4-6 hours
**Priority**: LOW

---

#### P4.2 Remove .bak Files
**Files**: Multiple .bak files
**Action**: Remove all .bak files from repository
- Use Git for history
- Clean up codebase

**Effort**: 1 hour
**Priority**: LOW

---

#### P4.3 Add Fallback Assets
**File**: `src/engine/resource/ResourceFallbackImpl.cpp`
**Action**: Create actual default assets
- Default texture (checkerboard)
- Default mesh (cube)
- Default material

**Effort**: 2-3 hours
**Priority**: LOW

---

#### P4.4 Clean Up Platform.h
**File**: `src/engine/Platform.h`
**Action**: Move platform-specific includes to PlatformWindows.cpp etc.
- Use proper IPlatform interface
- Remove #ifdef from core files

**Effort**: 2-4 hours
**Priority**: LOW

---

## Total Effort Estimation

| Phase | Tasks | Total Effort |
|--------|--------|---------------|
| Phase 1: Critical Fixes | 4 tasks | 7-10 hours |
| Phase 2: High-Priority | 6 tasks | 23-38 hours |
| Phase 3: Medium-Priority | 8 tasks | 32-55 hours |
| Phase 4: Low-Priority | 4 tasks | 9-14 hours |
| **Total** | **22 tasks** | **71-117 hours** (~9-15 days of focused work) |

---

## Execution Order

### Sprint 1 (Week 1-2) - Critical Fixes
1. Fix MeshAsset::Load()
2. Implement TextureAsset for desktop
3. Remove AudioDeviceXAudio2 stub
4. Fix XAudio2 memory leak
5. Enable SDL3 audio backend
6. Implement async loading (first pass)

### Sprint 2 (Week 3-4) - ECS Refactoring
1. Remove virtual functions from Component
2. Resolve GameObject vs Entity confusion
3. Fix ScriptSystem iteration bug
4. Add component views to ECS

### Sprint 3 (Week 5-6) - Features
1. Implement hot-reload
2. Implement RenderGraph
3. Complete SDL3 3D audio
4. Complete asset serialization

### Sprint 4 (Week 7-8) - Polish
1. Reduce singleton usage
2. Complete material loading
3. Add RAII wrappers
4. Remove .bak files
5. Add fallback assets
6. Implement upscalers
7. Complete VoxelRenderer

---

## Conclusion

Prisma Engine has a **solid foundation** with:
- ✅ Modern C++20 features
- ✅ Clean rendering interfaces
- ✅ ECS core with compile-time type IDs
- ✅ Cross-platform Vulkan support

But suffers from **hybrid architecture debt**:
- ❌ Mixed OOP/ECS paradigms
- ❌ Stub implementations masquerading as features
- ❌ Memory leaks and unsafe patterns
- ❌ Overuse of singletons

**Recommendation**: Execute Phase 1 (Critical Fixes) immediately, then proceed with Phase 2-4 systematically. The engine is worth saving, but requires focused refactoring to reach production quality.

---

**Document Version**: 1.0
**Last Updated**: 2026-03-09
**Next Review**: After Phase 1 completion
