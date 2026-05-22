# C# CoreCLR Scripting System — Design

> **Status:** Approved  
> **Architecture:** Unity-style (C# owns entities, C++ provides backend services)  
> **Runtime:** .NET CoreCLR via hostfxr  
> **Interop:** Bidirectional function pointer tables (GodotSharp-inspired)

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│ C++ PrismaEngine Engine Core                            │
│ (Rendering, Input, Asset, Audio, Physics)               │
│                                                         │
│  CoreCLRHost     ScriptEngine      PrismaAPI (C→C#)    │
│  ┌──────────┐   ┌───────────┐   ┌──────────────────┐   │
│  │ hostfxr  │   │ Bootstrap │   │ createEntity     │   │
│  │ init     │──▶│ OnFrame   │──▶│ setPosition      │   │
│  │ assembly │   │ lifecycle │   │ isKeyDown        │   │
│  └──────────┘   └───────────┘   └──────────────────┘   │
│                      │                 ▲                │
│        每帧 C#_OnFrame(dt)              │                │
└──────────────────────┼─────────────────┘                │
                       │                                  │
              CoreCLR Runtime                             │
                       │                                  │
┌──────────────────────┴──────────────────────────────────┘
│ C# PrismaEngine.Core.dll
│
│  Node       — 实体即位置 (替代 Unity's GameObject+Transform)
│  Script     — 脚本基类
│  Input      — 输入静态 API
│  Time       — 时间静态 API
│  Vector2    — 数学
│  Color      — 颜色
│  Random     — 随机
│  KeyCode    — 按键枚举
│
│ C# GameScripts.dll (用户项目)
│  SceneInit         — 场景入口（创建 Node + 附加脚本）
│  RotatingSprite    — 精灵旋转
│  CameraController  — WASD 相机移动
└──────────────────────────────────────────────────────────
```

## Design Decisions

### Entity Model: Node-only (no GameObject + Transform split)

| Unity Legacy | PrismaEngine |
|---|---|
| `GameObject go = new()` | `Node n = new("Name")` |
| `go.transform.position = ...` | `n.Position = ...` |
| `go.AddComponent<Rigidbody>()` | `n.AddScript<RotatingSprite>()` |
| `go.GetComponent<Renderer>()` | `n.GetScript<Renderer>()` |

### Naming Conventions (C# Side)

| Category | Rule | Example |
|---|---|---|
| Classes | PascalCase | `Node`, `Script`, `Input` |
| Properties | PascalCase | `node.Position`, `node.Rotation` |
| Methods | PascalCase | `node.AddScript<T>()` |
| Fields (serialized) | PascalCase | `float Speed = 45f` |
| Private fields | `_camelCase` | `_handle`, `_scripts` |
| Static APIs | PascalCase | `Time.DeltaTime`, `Input.GetKey()` |
| Enums | PascalCase | `KeyCode.W`, `KeyCode.Space` |

### Script Base Class

```csharp
public abstract class Script {
    public Node node { get; internal set; }
    public Vector2 Position { get => node.Position; set => node.Position = value; }
    public float Rotation { get => node.Rotation; set => node.Rotation = value; }

    public virtual void OnCreate() { }
    public virtual void OnStart() { }
    public virtual void OnUpdate(float dt) { }
    public virtual void OnDestroy() { }
}
```

Not "MonoBehaviour" — it's `Script`. The lifecycle is `OnCreate` / `OnStart` / `OnUpdate` / `OnDestroy`.

### C++ → C# Communication (every frame)

```
C++: Engine::Run() {
    m_scriptEngine.Update(dt);     // calls C#_OnFrame(dt)
    m_renderSystem->RenderFrame(); // renders current scene state
}
```

C# iterates all nodes, calls OnUpdate on their scripts. C++ then renders whatever state C# set up.

### C# → C++ Communication (engine services)

Function pointer table (`PrismaAPI`) passed at init. No DllImport, no symbol lookup at runtime.

### CoreCLR Hosting

- Use `hostfxr` + `nethost` for runtime initialization
- One assembly loaded: `GameScripts.dll` → references `PrismaEngine.Core.dll`
- Two entry points via `[UnmanagedCallersOnly]`:
  - `Bootstrap(IntPtr apiPtr)` — called once at init
  - `OnFrame(float dt)` — called every frame

## Project Structure (Template2D)

```
projects/Template2D/
├── assets/project.json
├── src/
│   ├── Template2DApp.cpp/.h    (simplified, no manual sprite logic)
│   └── main.cpp
└── scripts/
    ├── PrismaEngine.Core/
    │   ├── PrismaEngine.Core.csproj
    │   ├── Node.cs
    │   ├── Script.cs
    │   ├── Input.cs
    │   ├── Time.cs
    │   ├── Vector2.cs
    │   ├── Color.cs
    │   ├── Random.cs
    │   ├── KeyCode.cs
    │   └── EngineAPI.cs
    └── GameScripts/
        ├── GameScripts.csproj
        ├── SceneInit.cs
        ├── RotatingSprite.cs
        └── CameraController.cs
```

Engine side:

```
src/engine/scripting/
├── CoreCLRHost.h/.cpp      (NEW — replaces MonoRuntime)
└── ScriptEngine.h/.cpp     (NEW — script lifecycle management)
```

## C++ API (PrismaAPI — function pointer table)

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

## Implementation Order (verify after each step)

1. **CoreCLRHost** — hostfxr init + assembly load + Bootstrap/OnFrame entry points
2. **ScriptEngine** — C API implementation + entity lifecycle management
3. **PrismaEngine.Core C# project** — Node, Script, Input, Time, Vector2, Color, etc.
4. **GameScripts C# project** — SceneInit, RotatingSprite, CameraController
5. **Template2DApp integration** — wire ScriptEngine into Engine::Run, remove hardcoded sprite logic
6. **Build system** — CMake `add_custom_command` to build C# before native + runtimeconfig setup
