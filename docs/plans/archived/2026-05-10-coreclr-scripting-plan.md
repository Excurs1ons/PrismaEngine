# CoreCLR C# Scripting Implementation Plan

> **For Claude:** The user verifies after each task. Implement one task at a time.

**Goal:** Implement Unity-style C# scripting via CoreCLR for Prisma2D, replacing hardcoded C++ behaviors with C# scripts.

**Architecture:** C# owns game objects (Node class), C++ provides backend services via PrismaAPI function pointer table. CoreCLR hosted via hostfxr.

**Tech Stack:** .NET 10, CoreCLR hostfxr, C++20, CMake, Glaze (JSON)

---

### Task 1: CoreCLRHost — Runtime Initialization

**Files:**
- Create: `src/engine/scripting/CoreCLRHost.h`
- Create: `src/engine/scripting/CoreCLRHost.cpp`

**What it does:** Finds hostfxr.dll via DOTNET_ROOT, loads it via LoadLibrary, resolves `hostfxr_initialize_for_runtime_config`, `hostfxr_get_runtime_delegate`, `hostfxr_close`.

**Key detail:** No nethost dependency — probe `%DOTNET_ROOT%/host/fxr/*/hostfxr.dll` directly (simpler, no header dependency).

**Step 1: Create CoreCLRHost.h**

```cpp
#pragma once
#include <string>
#include <cstdint>

namespace Prisma::Scripting {

using hostfxr_handle = void*;

class CoreCLRHost {
public:
    CoreCLRHost() = default;
    ~CoreCLRHost() { Shutdown(); }

    bool Initialize(const std::string& runtimeConfigPath);
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    // Load assembly and get function pointer to a [UnmanagedCallersOnly] method
    void* GetFunctionPointer(const std::string& assemblyPath,
                             const std::string& typeName,
                             const std::string& methodName);

private:
    void* m_hostfxrLib = nullptr;
    hostfxr_handle m_context = nullptr;
    void* m_loadAssemblyAndGetFn = nullptr;
    bool m_initialized = false;
};

} // namespace Prisma::Scripting
```

**Step 2: Create CoreCLRHost.cpp**

```cpp
#include "CoreCLRHost.h"
#include "logger/Logger.h"
#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

namespace Prisma::Scripting {

// Minimal API signatures (from hostfxr.h)
using hostfxr_initialize_for_runtime_config_fn = int32_t (*)(
    const char_t*, const struct hostfxr_initialize_parameters*, hostfxr_handle*);
using hostfxr_get_runtime_delegate_fn = int32_t (*)(hostfxr_handle, int32_t, void**);
using hostfxr_close_fn = int32_t (*)(hostfxr_handle);
using load_assembly_and_get_function_pointer_fn = int32_t (*)(
    const char_t*, const char_t*, const char_t*, const char_t*, void*, void**);

#ifdef _WIN32
#define HOSTFXR_PATH_SEP '\\'
#define HOSTFXR_DLL "hostfxr.dll"
#else
#define HOSTFXR_PATH_SEP '/'
#define HOSTFXR_DLL "libhostfxr.so"
#endif

static std::string findHostfxrPath() {
    // Try DOTNET_ROOT env var first
    const char* dotnetRoot = getenv("DOTNET_ROOT");
    std::string root = dotnetRoot ? dotnetRoot : "";
    
    if (root.empty()) {
#ifdef _WIN32
        root = "C:\\Program Files\\dotnet";
#else
        root = "/usr/share/dotnet";
#endif
        if (!fs::exists(root)) root = "/usr/local/share/dotnet";
        if (!fs::exists(root)) return "";
    }
    
    // Probe: {root}/host/fxr/{version}/hostfxr.dll
    fs::path fxrDir = fs::path(root) / "host" / "fxr";
    if (!fs::exists(fxrDir)) return "";
    
    std::vector<fs::path> versions;
    for (const auto& entry : fs::directory_iterator(fxrDir)) {
        if (entry.is_directory()) versions.push_back(entry.path());
    }
    if (versions.empty()) return "";
    
    // Sort descending and take highest version
    std::sort(versions.begin(), versions.end(), std::greater<fs::path>());
    fs::path dllPath = versions[0] / HOSTFXR_DLL;
    
    return fs::exists(dllPath) ? dllPath.string() : "";
}

bool CoreCLRHost::Initialize(const std::string& runtimeConfigPath) {
    std::string fxrPath = findHostfxrPath();
    if (fxrPath.empty()) {
        LOG_ERROR("CoreCLRHost", "hostfxr.dll not found");
        return false;
    }
    
    m_hostfxrLib = LoadLibraryExA(fxrPath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!m_hostfxrLib) {
        LOG_ERROR("CoreCLRHost", "Failed to load hostfxr.dll");
        return false;
    }
    
    auto initFn = (hostfxr_initialize_for_runtime_config_fn)
        GetProcAddress((HMODULE)m_hostfxrLib, "hostfxr_initialize_for_runtime_config");
    auto getDelegateFn = (hostfxr_get_runtime_delegate_fn)
        GetProcAddress((HMODULE)m_hostfxrLib, "hostfxr_get_runtime_delegate");
    auto closeFn = (hostfxr_close_fn)
        GetProcAddress((HMODULE)m_hostfxrLib, "hostfxr_close");
    
    if (!initFn || !getDelegateFn || !closeFn) {
        LOG_ERROR("CoreCLRHost", "hostfxr exports not found");
        FreeLibrary((HMODULE)m_hostfxrLib);
        m_hostfxrLib = nullptr;
        return false;
    }
    
    // Initialize runtime from config
    int rc = initFn(runtimeConfigPath.c_str(), nullptr, &m_context);
    if (rc != 0 || !m_context) {
        LOG_ERROR("CoreCLRHost", "hostfxr_initialize_for_runtime_config failed: {0}", rc);
        closeFn(m_context);
        FreeLibrary((HMODULE)m_hostfxrLib);
        m_hostfxrLib = nullptr;
        return false;
    }
    
    // Get load_assembly_and_get_function_pointer delegate
    rc = getDelegateFn(m_context, 5 /*hdt_load_assembly_and_get_function_pointer*/,
                       &m_loadAssemblyAndGetFn);
    if (rc != 0 || !m_loadAssemblyAndGetFn) {
        LOG_ERROR("CoreCLRHost", "Failed to get load_assembly delegate");
        closeFn(m_context);
        FreeLibrary((HMODULE)m_hostfxrLib);
        m_hostfxrLib = nullptr;
        return false;
    }
    
    // Context can be closed now; delegate remains valid
    closeFn(m_context);
    m_context = nullptr;
    
    m_initialized = true;
    LOG_INFO("CoreCLRHost", "CoreCLR initialized (hostfxr: {0})", fxrPath);
    return true;
}

void* CoreCLRHost::GetFunctionPointer(const std::string& assemblyPath,
                                       const std::string& typeName,
                                       const std::string& methodName) {
    if (!m_initialized || !m_loadAssemblyAndGetFn) return nullptr;
    
    void* fn = nullptr;
    auto loadFn = (load_assembly_and_get_function_pointer_fn)m_loadAssemblyAndGetFn;
    
    int rc = loadFn(assemblyPath.c_str(), typeName.c_str(), methodName.c_str(),
                    (const char_t*)-1 /*UNMANAGEDCALLERSONLY_METHOD*/,
                    nullptr, &fn);
    
    if (rc != 0 || !fn) {
        LOG_ERROR("CoreCLRHost", "Failed to get function pointer: {0}::{1} (rc={2})",
                  typeName, methodName, rc);
        return nullptr;
    }
    
    return fn;
}

void CoreCLRHost::Shutdown() {
    m_initialized = false;
    m_loadAssemblyAndGetFn = nullptr;
    if (m_hostfxrLib) {
        FreeLibrary((HMODULE)m_hostfxrLib);
        m_hostfxrLib = nullptr;
    }
}

} // namespace Prisma::Scripting
```

**Note:** Linux/macOS will need `dlopen`/`dlsym` instead of `LoadLibraryExA`/`GetProcAddress`. For MVP, Windows only is fine — use `#ifdef _WIN32`.

**Step 3: Add to CMake**
In `src/engine/CMakeLists.txt`, add `scripting/CoreCLRHost.cpp` to `PRISMA_ENGINE_SOURCES` and `.h` to headers.

**Step 4: Verify build**
```bash
cmake --build build --target Engine --config Debug
```
Expected: compiles without errors (hostfxr loaded at runtime, no link dependency).

**Step 5: Commit**
```bash
git add src/engine/scripting/CoreCLRHost.* src/engine/CMakeLists.txt
```

---

### Task 2: ScriptEngine — C API + Lifecycle Management

**Files:**
- Create: `src/engine/scripting/ScriptEngine.h`
- Create: `src/engine/scripting/ScriptEngine.cpp`

**PrismaAPI** function pointer table:

```cpp
struct PrismaAPI {
    void (*log)(const char* subsystem, const char* msg);
    uint32_t (*createEntity)();
    void (*destroyEntity)(uint32_t id);
    void (*setPosition)(uint32_t id, float x, float y);
    void (*getPosition)(uint32_t id, float* x, float* y);
    void (*setRotation)(uint32_t id, float deg);
    float (*getRotation)(uint32_t id);
    void (*setScale)(uint32_t id, float x, float y);
    void (*setColor)(uint32_t id, float r, float g, float b, float a);
    void (*setSize)(uint32_t id, float w, float h);
    bool (*isKeyDown)(int key);
    float (*mouseX)();
    float (*mouseY)();
    float (*deltaTime)();
};
```

ScriptEngine:
```cpp
class ScriptEngine {
public:
    bool Initialize(CoreCLRHost& host);
    void Shutdown();
    void Update(float dt);
    bool IsInitialized() const { return m_initialized; }
    
    // C API implementations (static, called from C# via function pointer)
    static uint32_t CreateEntity();
    static void DestroyEntity(uint32_t id);
    static void SetPosition(uint32_t id, float x, float y);
    static void GetPosition(uint32_t id, float* x, float* y);
    static void SetRotation(uint32_t id, float deg);
    static float GetRotation(uint32_t id);
    static void SetScale(uint32_t id, float x, float y);
    static void SetColor(uint32_t id, float r, float g, float b, float a);
    static void SetSize(uint32_t id, float w, float h);
    static bool IsKeyDown(int key);
    static float MouseX();
    static float MouseY();
    static float DeltaTime();
    static void Log(const char* subsystem, const char* msg);
    
private:
    CoreCLRHost* m_host = nullptr;
    void (*m_bootstrapFn)() = nullptr;
    void (*m_onFrameFn)(float) = nullptr;
    bool m_initialized = false;
};
```

**Entity management:** C++ stores a map of `uint32_t → EntityData` (or just creates Prisma::GameObject in a temp scene). For MVP, keep it simple:
- Pre-allocate a pool of entities (max 1024)
- Each entity has position/rotation/scale/color/size
- Renderer reads from this pool each frame

Or even simpler for MVP: C++ create/destroy/store entities in a `std::vector`, and the existing RenderSystem renders them.

Actually, for the simplest MVP that works with Prisma2D: entities are just data structs stored in a C++ array. The render system reads from this array for rendering.

Wait, but Prisma2D uses Renderer2D immediate mode. The OnRender currently draws all sprites manually. After refactoring, the C++ scene would contain entities created by C# scripts.

The simplest approach for entity rendering: ScriptEngine stores entities in a vector that the existing render system iterates. Or, we make ScriptEngine::Update also responsible for rendering (since it knows all entities).

Actually, the cleanest approach: ScriptEngine stores entities. Prisma2DApp::OnRender iterates ScriptEngine's entities and draws them with Renderer2D. This way:
- C# scripts control entity data (position, rotation, color, size)
- C++ renders them each frame (no change to render pipeline)

For the C API implementations:
- `CreateEntity()`: allocate ID, store defaults in map
- `SetPosition(id, x, y)`: update map entry
- `IsKeyDown(key)`: delegate to InputManager
- etc.

---

### Task 3: PrismaEngine.Core C# Project

**Directory:** `projects/Prisma2D/scripts/PrismaEngine.Core/`

**Files to create:**
- `PrismaEngine.Core.csproj`
- `EngineAPI.cs`
- `Node.cs`
- `Script.cs`
- `Input.cs`
- `Time.cs`
- `Vector2.cs`
- `Color.cs`
- `Random.cs`
- `KeyCode.cs`

**PrismaEngine.Core.csproj:**
```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>
    <AssemblyName>PrismaEngine.Core</AssemblyName>
  </PropertyGroup>
</Project>
```

**EngineAPI.cs:**
```csharp
using System;
using System.Runtime.InteropServices;

namespace PrismaEngine {
    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct PrismaAPI {
        public delegate* unmanaged<byte*, byte*, void> Log;
        public delegate* unmanaged<uint> CreateEntity;
        public delegate* unmanaged<uint, void> DestroyEntity;
        public delegate* unmanaged<uint, float, float, void> SetPosition;
        public delegate* unmanaged<uint, float*, float*, void> GetPosition;
        public delegate* unmanaged<uint, float, void> SetRotation;
        public delegate* unmanaged<uint, float> GetRotation;
        public delegate* unmanaged<uint, float, float, void> SetScale;
        public delegate* unmanaged<uint, float, float, float, float, void> SetColor;
        public delegate* unmanaged<uint, float, float, void> SetSize;
        public delegate* unmanaged<int, bool> IsKeyDown;
        public delegate* unmanaged<float> MouseX;
        public delegate* unmanaged<float> MouseY;
        public delegate* unmanaged<float> DeltaTime;
    }

    internal static unsafe class NativeAPI {
        internal static PrismaAPI API;
        
        internal static void Init(PrismaAPI* api) {
            API = *api;
        }
    }
}
```

**Node.cs:**
```csharp
using System.Collections.Generic;

namespace PrismaEngine {
    public class Node {
        internal uint _handle;
        internal List<Script> _scripts = new();
        
        public string Name { get; set; }
        
        public float X {
            get { float x = 0, y = 0; unsafe { NativeAPI.API.GetPosition(_handle, &x, &y); } return x; }
            set { unsafe { NativeAPI.API.SetPosition(_handle, value, Y); } }
        }
        public float Y {
            get { float x = 0, y = 0; unsafe { NativeAPI.API.GetPosition(_handle, &x, &y); } return y; }
            set { unsafe { NativeAPI.API.SetPosition(_handle, X, value); } }
        }
        public Vector2 Position {
            get { float x = 0, y = 0; unsafe { NativeAPI.API.GetPosition(_handle, &x, &y); } return new(x, y); }
            set { unsafe { NativeAPI.API.SetPosition(_handle, value.X, value.Y); } }
        }
        public float Rotation {
            get { unsafe { return NativeAPI.API.GetRotation(_handle); } }
            set { unsafe { NativeAPI.API.SetRotation(_handle, value); } }
        }
        public Vector2 Scale {
            get { float x = 0, y = 0; ... /* reuse GetPosition as placeholder */ return new(x, y); }
            set { unsafe { NativeAPI.API.SetScale(_handle, value.X, value.Y); } }
        }
        
        public Node(string name) {
            unsafe { _handle = NativeAPI.API.CreateEntity(); }
            Name = name;
            ScriptEngine.RegisterNode(this);
        }
        
        public T AddScript<T>() where T : Script, new() {
            var s = new T { node = this };
            _scripts.Add(s);
            s.OnCreate();
            return s;
        }
        
        public T GetScript<T>() where T : Script {
            foreach (var s in _scripts) if (s is T t) return t;
            return null;
        }
    }
}
```

**Script.cs:**
```csharp
namespace PrismaEngine {
    public abstract class Script {
        public Node node { get; internal set; }
        public float X { get => node.X; set => node.X = value; }
        public float Y { get => node.Y; set => node.Y = value; }
        public Vector2 Position { get => node.Position; set => node.Position = value; }
        public float Rotation { get => node.Rotation; set => node.Rotation = value; }
        
        internal bool _started = false;
        
        public virtual void OnCreate() { }
        public virtual void OnStart() { }
        public virtual void OnUpdate(float dt) { }
        public virtual void OnDestroy() { }
    }
}
```

**ScriptEngine.cs** (static, with `[UnmanagedCallersOnly]` entry points):
```csharp
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace PrismaEngine {
    internal static class ScriptEngine {
        static List<Node> _allNodes = new();
        
        internal static void RegisterNode(Node n) => _allNodes.Add(n);
        
        [UnmanagedCallersOnly]
        public static void Bootstrap(IntPtr apiPtr) {
            unsafe { NativeAPI.Init((PrismaAPI*)apiPtr); }
        }
        
        [UnmanagedCallersOnly]
        public static void OnFrame(float dt) {
            Time.DeltaTime = dt;
            Time.Elapsed += dt;
            
            foreach (var n in _allNodes) {
                for (int i = n._scripts.Count - 1; i >= 0; i--) {
                    var s = n._scripts[i];
                    if (!s._started) { s.OnStart(); s._started = true; }
                    s.OnUpdate(dt);
                }
            }
        }
    }
}
```

**Input.cs:**
```csharp
namespace PrismaEngine {
    public enum KeyCode {
        W = 26, A = 4, S = 22, D = 7,
        Up = 82, Down = 81, Left = 80, Right = 79,
        Space = 44, Escape = 41
    }
    
    public static class Input {
        public static unsafe bool GetKey(KeyCode key) => NativeAPI.API.IsKeyDown((int)key);
        public static unsafe Vector2 MousePosition => new(NativeAPI.API.MouseX(), NativeAPI.API.MouseY());
    }
}
```

**Time.cs:**
```csharp
namespace PrismaEngine {
    public static class Time {
        public static float DeltaTime { get; internal set; }
        public static float Elapsed { get; internal set; }
    }
}
```

**Vector2.cs:**
```csharp
using System;
namespace PrismaEngine {
    public struct Vector2 { public float X, Y; ... }
}
```

**Color.cs:**
```csharp
namespace PrismaEngine {
    public struct Color { public float R, G, B, A; ... }
}
```

**Random.cs:**
```csharp
namespace PrismaEngine {
    public static class Random {
        static System.Random _rng = new();
        public static float Range(float min, float max) => (float)(_rng.NextDouble() * (max - min) + min);
        public static int Range(int min, int max) => _rng.Next(min, max);
    }
}
```

**Build:**
```bash
cd scripts/PrismaEngine.Core && dotnet build -c Release
```

---

### Task 4: GameScripts C# Project

**Directory:** `projects/Prisma2D/scripts/GameScripts/`

**Files:**
- `GameScripts.csproj` (references PrismaEngine.Core)
- `ScriptEntry.cs` (Bootstrap entry point called by C++)
- `SceneInit.cs`
- `RotatingSprite.cs`
- `CameraController.cs`

**GameScripts.csproj:**
```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>
    <AssemblyName>GameScripts</AssemblyName>
  </PropertyGroup>
  <ItemGroup>
    <Reference Include="PrismaEngine.Core">
      <HintPath>../PrismaEngine.Core/bin/Release/net10.0/PrismaEngine.Core.dll</HintPath>
    </Reference>
  </ItemGroup>
</Project>
```

**ScriptEntry.cs:**
```csharp
using System.Runtime.InteropServices;
using PrismaEngine;

namespace GameScripts {
    internal static class ScriptEntry {
        [UnmanagedCallersOnly]
        public static void Bootstrap(IntPtr apiPtr) {
            ScriptEngine.Bootstrap(apiPtr);
            
            // Create scene
            var init = new Node("__SceneInit__");
            init.AddScript<SceneInit>();
        }
        
        [UnmanagedCallersOnly]
        public static void OnFrame(float dt) {
            ScriptEngine.OnFrame(dt);
        }
    }
}
```

**SceneInit.cs:**
```csharp
using PrismaEngine;

namespace GameScripts {
    public class SceneInit : Script {
        public override void OnCreate() {
            // Camera
            var cam = new Node("MainCamera");
            cam.Position = new Vector2(960, 540);
            cam.AddScript<CameraController>();
            
            // 20 dynamic sprites
            for (int i = 0; i < 20; i++) {
                var n = new Node($"Sprite_{i}");
                n.Position = new Vector2(
                    Random.Range(0, 1920), Random.Range(0, 1080));
                n.Scale = new Vector2(
                    Random.Range(40, 100), Random.Range(40, 100));
                n.AddScript<RotatingSprite>();
            }
        }
    }
}
```

**RotatingSprite.cs:**
```csharp
using PrismaEngine;

namespace GameScripts {
    public class RotatingSprite : Script {
        public float Speed = 45f;
        
        public override void OnCreate() {
            Rotation = Random.Range(0, 360);
        }
        
        public override void OnUpdate(float dt) {
            Rotation += Speed * dt;
            if (Rotation > 360) Rotation -= 360;
        }
    }
}
```

**CameraController.cs:**
```csharp
using PrismaEngine;

namespace GameScripts {
    public class CameraController : Script {
        public float MoveSpeed = 5f;
        
        public override void OnUpdate(float dt) {
            var p = Position;
            if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.Up))    p.Y += MoveSpeed * dt;
            if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.Down))  p.Y -= MoveSpeed * dt;
            if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.Left))  p.X -= MoveSpeed * dt;
            if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.Right)) p.X += MoveSpeed * dt;
            Position = p;
        }
    }
}
```

---

### Task 5: Prisma2DApp + Engine Integration

**Files to modify:**
- `src/engine/app/Engine.h` — add `ScriptEngine` member, `GetScriptEngine()`
- `src/engine/app/Engine.cpp` — `Run()`: init CoreCLRHost → ScriptEngine → Bootstrap → OnFrame per loop
- `projects/Prisma2D/src/Prisma2DApp.cpp` — **major cleanup**: remove all TestSprite generation/rotation logic, replace with ScriptEngine-based rendering
- `projects/Prisma2D/src/Prisma2DApp.h` — remove TestSprite struct, FPS/GPU members (already done)

**Engine.h additions:**
```cpp
#include "scripting/CoreCLRHost.h"
#include "scripting/ScriptEngine.h"

class Engine {
    // ...
    Scripting::CoreCLRHost& GetCoreCLRHost() { return m_coreCLRHost; }
    Scripting::ScriptEngine& GetScriptEngine() { return m_scriptEngine; }
    // ...
private:
    Scripting::CoreCLRHost m_coreCLRHost;
    Scripting::ScriptEngine m_scriptEngine;
};
```

**Engine.cpp Run() integration:**
```cpp
void Engine::Run() {
    // ... existing init (window, render, etc.) ...
    
    // Initialize C# scripting
    std::string scriptsDir = /* find GameScripts.dll dir */;
    std::string configPath = scriptsDir + "/GameScripts.runtimeconfig.json";
    
    if (m_coreCLRHost.Initialize(configPath)) {
        m_scriptEngine.Initialize(m_coreCLRHost);
        LOG_INFO("Engine", "C# scripting initialized");
    } else {
        LOG_WARNING("Engine", "C# scripting not available (will run without scripts)");
    }
    
    // Main loop
    while (m_Running) {
        float dt = ...;
        
        // Update C# scripts
        if (m_scriptEngine.IsInitialized())
            m_scriptEngine.Update(dt);
        
        // App update
        if (m_CurrentApp) m_CurrentApp->OnUpdate(ts);
        
        // Render
        BeginFrame();
        if (m_CurrentApp) m_CurrentApp->OnRender();
        EndFrame();
    }
}
```

**Prisma2DApp.cpp — OnInitialize (simplified):**
```cpp
int Prisma2DApp::OnInitialize() {
    // C# Scripting bootstraps everything in ScriptEntry.Bootstrap
    // No manual sprite creation needed
    LOG_INFO("Prisma2D", "C# 脚本化初始化完成, 分辨率={0}x{1}", m_Spec.Width, m_Spec.Height);
    return 0;
}
```

**Prisma2DApp.cpp — OnRender (draw entities from ScriptEngine):**
```cpp
void Prisma2DApp::OnRender() {
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    auto ortho = std::dynamic_pointer_cast<Graphic::OrthographicCamera>(camera);
    if (!ortho) return;
    
    Graphic::Renderer2D::BeginScene(*ortho);
    
    // Grid lines (kept in C++, it's engine-level)
    // ...
    
    // Draw entities created by C# scripts
    auto& scriptEngine = Engine::Get().GetScriptEngine();
    for (uint32_t id = 0; id < scriptEngine.GetEntityCount(); id++) {
        auto* e = scriptEngine.GetEntity(id);
        if (!e || !e->active) continue;
        
        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(e->position, 0.0f));
        t = glm::rotate(t, glm::radians(e->rotation), glm::vec3(0, 0, 1));
        t = glm::scale(t, glm::vec3(e->size, 1.0f));
        Graphic::Renderer2D::DrawQuad(t, e->color);
    }
    
    // Diagnostics (use Engine::GetFPS/GetGPUName)
    // ...
    
    Graphic::Renderer2D::EndScene();
}
```

**Note:** For Step 5 to work, `ScriptEngine` needs an `EntityData` array + accessors:

```cpp
struct EntityData {
    bool active = false;
    Vector2 position;
    float rotation = 0.0f;
    Vector2 scale = {1, 1};
    Color color = {1,1,1,1};
    Vector2 size = {50, 50};
};

class ScriptEngine {
public:
    EntityData* GetEntity(uint32_t id) {
        return (id < m_entities.size()) ? &m_entities[id] : nullptr;
    }
    uint32_t GetEntityCount() const { return (uint32_t)m_entities.size(); }
    
private:
    std::vector<EntityData> m_entities;
    static const uint32_t kMaxEntities = 1024;
    
    static uint32_t CreateEntityImpl() {
        auto& engine = Engine::Get().GetScriptEngine();
        for (uint32_t i = 0; i < kMaxEntities; i++) {
            if (!engine.m_entities[i].active) {
                engine.m_entities[i].active = true;
                return i;
            }
        }
        return 0; // pool exhausted
    }
};
```

---

### Task 6: Build System — CMake Integration

**Files to modify:**
- `projects/Prisma2D/CMakeLists.txt` — add `add_custom_command` to build C# projects
- Optionally: script to find dotnet SDK path

**CMake snippet:**
```cmake
# Find dotnet
find_program(DOTNET dotnet REQUIRED)

# Build C# projects
set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

add_custom_command(
    OUTPUT ${SCRIPTS_DIR}/PrismaEngine.Core/bin/Release/net10.0/PrismaEngine.Core.dll
    COMMAND ${DOTNET} build -c Release
    WORKING_DIRECTORY ${SCRIPTS_DIR}/PrismaEngine.Core
    COMMENT "Building PrismaEngine.Core (C#)"
)

add_custom_command(
    OUTPUT ${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/GameScripts.dll
    COMMAND ${DOTNET} build -c Release
    WORKING_DIRECTORY ${SCRIPTS_DIR}/GameScripts
    DEPENDS ${SCRIPTS_DIR}/PrismaEngine.Core/bin/Release/net10.0/PrismaEngine.Core.dll
    COMMENT "Building GameScripts (C#)"
)

# Copy C# assemblies to output
add_custom_target(BuildScripts ALL
    DEPENDS
        ${SCRIPTS_DIR}/PrismaEngine.Core/bin/Release/net10.0/PrismaEngine.Core.dll
        ${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/GameScripts.dll
)

add_dependencies(Prisma2D BuildScripts)

# Copy to build output
file(COPY ${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scripts/)
```
