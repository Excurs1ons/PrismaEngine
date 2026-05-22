# Prisma MCP — 内置 Agent 协议支持

- 日期: 2026-05-09
- 状态: 设计阶段 / 待实现
- 涉及团队: 引擎核心

## 1. 概述

为 Prisma Engine 添加内置 MCP (Model Context Protocol) 支持，使 AI 编辑器（Claude Code、Cursor 等）和 LLM Agent 能通过标准化协议实时查询和控制引擎状态。

### 核心目标

- **内置而非插件** — 编译进引擎核心，作为 `ISubSystem` 注册
- **极致 Token 优化** — 内容哈希增量追踪 + 字段过滤 + 默认值省略 + 分页
- **底层详细接口** — 不限于场景层级，覆盖 ECS 组件、GPU 资源、性能计数器等
- **编译-运行闭环** — Agent 修改代码 → 编译 → 启动引擎 → 自动重连 → 增量恢复 → 验证

### 对比 Unity MCP

| 维度 | Unity MCP | Prisma MCP |
|------|-----------|------------|
| 集成方式 | 第三方 NuGet 包，反射调用 | 编译进引擎，`ISubSystem` 注册，零反射 |
| Token 优化 | 无 | 增量/字段过滤/默认值省略/分页 |
| 通信 | 仅 stdio | stdio + TCP/SSE 双模式 |
| 会话管理 | 无状态 | 内容哈希增量，进程重启可恢复 |
| 覆盖面 | 编辑器操作为主 | 引擎 + 编辑器 + 运行时 + GPU/调试 |
| 工具扩展 | 固定工具集 | 分层注册，任意模块可注册工具 |

## 2. 架构

### 组件图

```
Engine
└── MCPSubSystem (ISubSystem)
    ├── MCPServer
    │   ├── Transport (stdio | tcp)
    │   ├── Session (hash delta + token budget)
    │   └── ToolRegistry
    │       ├── SceneTools
    │       ├── ECSTools
    │       ├── EngineTools
    │       ├── DebugTools
    │       ├── AssetTools
    │       ├── [Editor] EditorTools
    │       └── [Game] GameTools
    └── DeltaTracker
```

### 依赖

- **nlohmann-json** — 已有依赖，JSON-RPC 消息序列化
- **xxhash (xxh3_64)** — 新增依赖，增量追踪哈希树
- **TCP transport** — OS socket API（Winsock2 / POSIX），无第三方依赖

## 3. 核心设计

### 3.1 传输层

```cpp
class MCPTransport {
public:
    virtual bool Start(MessageHandler handler) = 0;
    virtual void Stop() = 0;
    virtual bool Send(const nlohmann::json& msg) = 0;
    virtual bool IsConnected() const = 0;
    using MessageHandler = std::function<void(const nlohmann::json&)>;
};
```

两种实现：
- **TransportStdio** — 同步读写 stdin/stdout，子进程模式
- **TransportTCP** — TCP socket + SSE，独立进程/远程调试模式

### 3.2 工具注册系统

```cpp
class MCPTool {
public:
    virtual std::string_view GetName() const = 0;
    virtual std::string_view GetDescription() const = 0;
    virtual nlohmann::json GetInputSchema() const = 0;
    virtual nlohmann::json Execute(const nlohmann::json& args) = 0;
    virtual bool SupportsFieldFilter() const { return false; }
};
```

分层注册：引擎核心层 → 编辑器层 → 游戏层，通过 `MCPSubSystem::RegisterTool<T>()` 注入。

### 3.3 内容哈希增量追踪

使用 **xxh3_64** 为每个可追踪状态单元生成非加密哈希指纹：

```
Entity → serialized json → xxh3_64 → entity_hash
Scene  → sort(entity_hashes) → scene_hash
Engine → concat(scene_hash + asset_hash + ...) → root_hash
```

Agent 重连时发送已知 hash，引擎比较后决策增量/全量：

```python
def respond(known_hash, current_hash):
    if known_hash == current_hash:
        return no_changes()
    change_ratio = compute_change_ratio(known_hash, current_hash)
    if change_ratio > 0.3:
        return full_snapshot()    # 差异太大，diff 浪费 token
    else:
        return delta_response()   # 细粒度增量
```

### 3.4 Token 优化

1. **字段过滤**: 所有查询工具支持 `fields` 参数，只请求必要字段
2. **默认值省略**: `ComponentSerializer` 识别默认值，返回空对象
3. **增量响应**: 首次全量 + 后续只发差异
4. **智能分页**: `per_page` + `page` 参数控制响应大小
5. **Token 预算**: Agent 在 handshake 声明 `max_tokens_per_response`，引擎自动降级

### 3.5 会话与重连

Agent 无需持久化状态，首次连接时 handshake 携带上次的 `root_hash`：

```
Agent → Engine:  connect(root_hash: "0xA3F7C21D")
Engine → Agent:  delta or full_snapshot
```

引擎重启不影响 — 哈希由内容决定而非进程生命周期。

## 4. 工具清单

### Engine 核心工具（始终可用）

| 工具 | 描述 | 优化 |
|------|------|------|
| `scene_get_hierarchy` | 场景实体树 | field_filter, hash_delta |
| `scene_get_entity` | 实体详情 | field_filter |
| `scene_create_entity` | 创建实体 | — |
| `scene_delete_entity` | 删除实体 | — |
| `ecs_component_list` | 列出组件类型 | — |
| `ecs_component_get` | 获取组件数据 | field_filter, omit_defaults |
| `ecs_component_set` | 设置组件属性 | — |
| `ecs_component_add/remove` | 增减组件 | — |
| `ecs_archetype_query` | 按组件组合查实体 | pagination |
| `asset_list` | 资源列表 | pagination |
| `asset_get_info` | 资源元数据 | — |
| `engine_get_status` | 引擎状态 | — |
| `engine_get_state_hash` | 根状态哈希 | **增量入口** |
| `mcp_discover_all_tools` | 发现所有工具 | 自动调用 |

### 编辑器工具（Editor 编译时可用）

| 工具 | 描述 |
|------|------|
| `editor_get_selection` | 当前选中实体 |
| `editor_set_selection` | 选中实体 |
| `editor_get_viewport_info` | 视口相机信息 |
| `editor_console_get` | 控制台输出 |

### 调试工具（非 Headless 可用）

| 工具 | 描述 |
|------|------|
| `debug_frame_stats` | 帧统计 (FPS, draw calls) |
| `debug_memory_stats` | 内存统计 |
| `debug_gpu_resources` | GPU 资源列表 |
| `debug_performance` | 性能分析器 |

### 游戏运行时工具（Game 注册）

| 工具 | 描述 |
|------|------|
| `game_get_state` | 游戏运行状态 |
| `game_simulate` | 推进 N 帧 |
| `game_get_input_state` | 输入状态 |
| `game_override_variable` | 运行时变量覆写 |

## 5. 编译-运行循环

```
Agent: 修改代码
Agent: cmake --build --preset editor-windows-x64-debug
Agent: ./bin/PrismaEditor --mcp --mcp-transport=stdio
       │
       ▼
引擎启动 → MCP SubSystem 初始化 → 监听 stdio
       │
Agent: connect + handshake (含上次 root_hash)
Agent: 获取增量 → 决策 → 建议下一轮修改
       │
       ▼ (重复直到满意)
修改代码 → 编译 → 重启引擎 → 自动重连 → 增量恢复
```

使用内容哈希后，Agent 无需持久化任何状态。root_hash 由内容决定，引擎重启后重新计算并与 Agent 提供的旧 hash 比对即可。

## 6. 文件结构

```
src/engine/mcp/
├── CMakeLists.txt
├── MCPSubSystem.h/.cpp
├── MCPServer.h/.cpp
├── MCPSession.h/.cpp
├── MCPTool.h/.cpp
├── transport/
│   ├── Transport.h
│   ├── TransportStdio.h/.cpp
│   └── TransportTCP.h/.cpp
├── serialization/
│   ├── MCPJson.h/.cpp
│   └── ComponentSerializer.h/.cpp
├── session/
│   ├── DeltaTracker.h/.cpp
│   └── TokenBudget.h/.cpp
└── tools/
    ├── SceneTools.h/.cpp
    ├── ECSTools.h/.cpp
    ├── AssetTools.h/.cpp
    ├── DebugTools.h/.cpp
    ├── EngineTools.h/.cpp
    ├── EditorTools.h/.cpp
    └── GameTools.h/.cpp
```

## 7. 实施阶段

| Phase | 内容 | 估计 |
|-------|------|------|
| 1 | MCP 协议核心 + Transport | ~1 天 |
| 2 | 工具系统 + 核心工具 | ~1.5 天 |
| 3 | Session + 哈希增量 | ~1 天 |
| 4 | 编辑器/调试工具 | ~0.5 天 |
| 5 | Asset/Game 工具 + 编译循环 | ~0.5 天 |
| 6 | 测试 + 文档 | ~0.5 天 |
| **总计** | | **~5 天** |

## 8. 编译集成

`cmake/DeviceOptions.cmake`:

```cmake
option(PRISMA_ENABLE_MCP "Enable MCP server for AI agent support" ON)
```

`src/engine/CMakeLists.txt`:

```cmake
if(PRISMA_ENABLE_MCP)
    FetchContent_MakeAvailable(xxhash)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_MCP)
    target_link_libraries(Engine PRIVATE xxhash::xxhash)
    target_sources(Engine PRIVATE ${MCP_SOURCES})
endif()
```

## 9. 启动方式

```bash
# 开发模式: Agent 通过 stdio 管理子进程
./bin/PrismaEditor --mcp --mcp-transport=stdio

# 独立进程模式: TCP 远程连接
./bin/PrismaEditor --mcp --mcp-transport=tcp --mcp-port=3100

# Runtime 模式 (无编辑器, 通过 TCP 连接)
./bin/PrismaRuntime --mcp --mcp-transport=tcp --mcp-port=3100
```

## 10. 开放问题

- TCP transport 是否需要认证令牌？
- 运行时模式是否需要限定工具范围（只暴露 game_* 工具）？
- 是否需要支持 HTTP SSE 传输以兼容更多 MCP 客户端？
