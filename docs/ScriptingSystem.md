# 脚本系统设计

> **状态**: ✅ 基础实现完成（CoreCLR 后端）
> **优先级**: 高
> **依赖**: .NET 10 SDK, hostfxr

## 概述

PrismaEngine 的脚本系统使用 C# 编写游戏逻辑，通过 **CoreCLR 宿主（hostfxr）** 或 **Mono 运行时** 与引擎核心交互。运行时支持通过编译时选项和项目配置灵活切换。

## 设计目标

- 支持 C# 作为主要脚本语言（CoreCLR / Mono 双后端）
- 编译时开关控制脚本子系统是否编译
- 项目级别运行时开关控制脚本是否激活
- 通过 `PrismaAPI` 函数指针表实现 C# ↔ C++ 双向调用
- 轻量级实体数据池（2048 个实体上限），不依赖 ECS

## 后端支持

| 后端 | CMake 值 | 状态 | 说明 |
|------|----------|------|------|
| 关闭 | `OFF` | ✅ | 不编译脚本代码，纯 Native 执行 |
| CoreCLR | `CORECLR` | ✅ 默认 | 通过 `hostfxr.dll` 自承载 .NET 运行时 |
| Mono | `MONO` | 🔧 预留 | Mono 运行时嵌入（`#ifdef PRISMA_ENABLE_MONO` 保护） |

## 架构设计

```
┌─────────────────────────────────────────┐
│          C# Game Scripts               │
│  (GameScripts.dll, [UnmanagedCallersOnly]) │
├─────────────────────────────────────────┤
│         CoreCLRHost / MonoRuntime       │
│  (hostfxr 加载 / Mono JIT 嵌入)         │
├─────────────────────────────────────────┤
│              ScriptEngine               │
│  (PrismaAPI 函数指针表 + 实体池管理)     │
├─────────────────────────────────────────┤
│               Engine Core               │
│  (Renderer, Input, Audio, Physics...)   │
└─────────────────────────────────────────┘
```

## 互操作机制

C# → C++：通过 `PrismaAPI` 函数指针结构体（log, createEntity, isKeyDown 等）
C++ → C#：通过 `[UnmanagedCallersOnly]` 导出函数（Bootstrap, OnFrame）

```cpp
struct PrismaAPI {
    uint32_t (*createEntity)();
    void (*setPosition)(uint32_t id, float x, float y);
    bool (*isKeyDown)(int key);
    // ...
};
```

## 编译时配置 (CMake)

```cmake
# 默认启用 CoreCLR
cmake --preset engine-windows-x64-debug

# 关闭脚本
cmake --preset engine-windows-x64-debug -DPRISMA_ENABLE_SCRIPTING=OFF

# Mono 模式
cmake --preset engine-windows-x64-debug -DPRISMA_ENABLE_SCRIPTING=MONO
```

编译时 `PRISMA_ENABLE_SCRIPTING` 定义为：
- `0` = OFF（不编译 `scripting/` 目录下的源文件）
- `1` = MONO（同时定义 `PRISMA_ENABLE_MONO`）
- `2` = CORECLR

## 运行时配置 (project.json)

```json
{
    "name": "MyGame",
    "entryScene": "assets/scenes/main.json",
    "scriptingBackend": "CoreCLR",
    "window": { ... }
}
```

`scriptingBackend` 取值：`"Off"` / `"Mono"` / `"CoreCLR"`（默认 `"CoreCLR"`）

运行时行为：
- **Off**: 跳过脚本子系统，不初始化运行时，不加载 DLL
- **Mono**: 尝试 Mono 初始化（需要编译时启用 MONO）
- **CoreCLR**: 搜索 `scripts/` 目录，通过 `hostfxr.dll` 加载 .NET 程序集

## 初始化流程

```
Engine::Run()
  └─ 读取 project.json → scriptingBackend
  └─ if scriptingBackend == CoreCLR:
       ├─ 搜索 scripts/GameScripts.runtimeconfig.json
       ├─ CoreCLRHost::Initialize(scriptsDir)
       │    ├─ 加载 hostfxr.dll
       │    ├─ 初始化 .NET 运行时
       │    └─ 获取 Bootstrap 函数指针
       └─ ScriptEngine::Initialize(*host)
            ├─ 调用 C# Bootstrap(api)
            └─ 获取 OnFrame 函数指针
  └─ Main Loop:
       └─ if ScriptEngine::IsInitialized():
            └─ ScriptEngine::Update(dt) → 调用 C# OnFrame(dt)
```

## 代码位置

| 文件 | 职责 |
|------|------|
| `src/engine/scripting/CoreCLRHost.h/.cpp` | CoreCLR 运行时宿主（hostfxr） |
| `src/engine/scripting/MonoRuntime.h/.cpp` | Mono 运行时封装（含占位桩代码） |
| `src/engine/scripting/ScriptEngine.h/.cpp` | 脚本引擎：PrismaAPI + 实体池 |
| `src/engine/scripting/ScriptSystem.h/.cpp` | ECS 集成（Mono 路径） |
| `src/engine/app/Engine.h/.cpp` | 引擎初始化中的条件编译逻辑 |
| `src/engine/app/ProjectConfig.h` | `ScriptingBackend` 枚举 + JSON 序列化 |
| `cmake/DeviceOptions.cmake` | CMake 选项定义 |

## 开发计划

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | CoreCLR 宿主集成 | ✅ 完成 |
| Phase 2 | 脚本后端开关（编译+运行） | ✅ 完成 |
| Phase 3 | Mono 运行时初始化 | ⏳ 待实现 |
| Phase 4 | ECS 全集成 | ⏳ 待实现 |
| Phase 5 | 热重载支持 | ⏳ 待实现 |

## 开发建议

- **IDE**: 推荐使用 Visual Studio 2022 (Windows) 或 VS Code + C# Dev Kit (跨平台)。
- **编译 Termux 环境**: 
  在 Android/Termux 或受限虚拟内存环境（`ulimit -v`）中编译时，.NET 10 默认尝试预留 256GB 虚拟地址空间，这会导致编译失败。必须通过以下环境变量压制：
  ```bash
  export DOTNET_GCRegionRange=0x10000000
  dotnet build projects/Template2D/scripts/GameScripts/GameScripts.csproj
  ```

## 参考资料

- [.NET Host](https://learn.microsoft.com/en-us/dotnet/core/tutorials/nethost)
- [Mono Embedding](https://www.mono-project.com/docs/embedding/)
- [DeviceConfiguration.md](DeviceConfiguration.md)

---

*文档创建时间: 2025-12-25 | 最后更新: 2026-05-10*
