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

## 内存架构

### SoA 池：虚拟内存预留 + 按需提交

实体数据采用 **SoA（Structure of Arrays）** 布局，双缓冲（Transform A/B）支持并行写入。内存管理使用 **Virtual Memory Reservation** 而非固定分配或 `std::vector`：

```
启动时 VirtualAlloc(MEM_RESERVE) 预留 1M 实体虚拟地址（不占物理内存）
  ├─ Transform A: 5 float 数组 × 4MB = 20MB VA
  ├─ Transform B: 5 float 数组 × 4MB = 20MB VA
  └─ Render:     8 数组   × 4MB = 32MB VA（无缓冲）
                   总 VA 预留: 72MB（仅页表，无物理内存）

CreateEntity 时按需 VirtualAlloc(MEM_COMMIT)
  以 kCommitStep=16384 槽位为粒度提交物理页
  20 实体时仅提交 ~288KB 物理内存
```

**设计取舍：**

| 方案 | 指针稳定性 | 物理内存浪费 | 扩容成本 |
|------|-----------|-------------|---------|
| `float posX[32768]` 固定数组 | ✅ 永远不变 | ❌ 640KB 固定浪费 | ❌ 越界=崩溃 |
| `std::vector<float>` | ❌ 扩容即野指针 | ✅ 按需分配 | ❌ 需同步 C# 指针 |
| `VirtualAlloc(MEM_RESERVE)` | ✅ 终身固定 | ✅ 按需提交 | ✅ 页粒度提交，指针不变 |

**Active Range Copy：**
Bootstrap 后将活跃实体区间（0 ~ `m_aliveCount`）从 Write 缓冲区镜像到 Read 缓冲区：
```cpp
// 20 个实体时只拷贝 20 × 4 × 5 = 400 字节
std::memcpy(m_layoutA.posX, m_layoutB.posX, m_aliveCount * sizeof(float));
```
而非全量 640KB 或全 VA 范围拷贝。

**关键指标：**

| 场景 | 固定 32768 | std::vector | VirtualAlloc (当前) |
|------|-----------|-------------|-------------------|
| 20 实体物理内存 | 640KB | ~2KB | ~288KB |
| 1M 实体物理内存 | 无法支持 | 20MB + vector 元数据 | 20MB（无元数据） |
| 扩容后指针稳定性 | N/A | ❌ 需同步 | ✅ 不变 |
| C# 边界检查 | 硬编码常量 | 动态查询 | AliveCount 查询 |

### 实体句柄

```
31        16 15        0
┌────────────┬──────────┐
│ generation │  index   │
└────────────┴──────────┘
```

- `index`（低 16 位）：SoA 数组索引，最大 65535
- `generation`（高 16 位）：槽位复用计数，用于检测野指针（stale handle）

DestroyEntity 递增 generation，CreateEntity 重用空闲槽时返回新 generation。
持有旧句柄的 Node 在 Validate() 中检测 generation 不匹配并抛出异常。

### 关键文件

| 文件 | 职责 |
|------|------|
| `ScriptEngine.h` | SoA 结构体定义、VirtualAlloc VA 块 |
| `ScriptEngine.cpp` | `osReserve/Commit/Release`、`CreateEntity` 自动扩容、`commitRange`、Active Range Copy |
| `EngineAPI.cs` | C# 侧 `float*` 指针匹配、`GetEntityCapacity` 动态边界 |

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
