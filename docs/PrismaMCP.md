# Prisma MCP — 内置 AI Agent 协议

## 概述

Prisma MCP 是 Prisma Engine 内置的 MCP (Model Context Protocol) 服务器实现，允许 AI 编辑器（Claude Code、Cursor 等）通过标准化协议实时查询和控制引擎状态。

### 核心特性

- **内置集成** — 编译进编辑器层 (PrismaEditor)，零外部依赖
- **Token 优化** — 内容哈希增量追踪 / 字段过滤 / 默认值省略 / 智能分页
- **底层接口** — 场景层级、ECS 组件、GPU 资源、性能计数器全覆盖
- **双传输模式** — stdio（子进程）和 TCP（独立进程），支持编译-运行循环
- **分层工具注册** — 引擎层 → 编辑器层 → 游戏层，各司其职

## 启动方式

### Stdio 模式（推荐，开发编辑）

```bash
# Agent 通过子进程方式管理引擎
./bin/PrismaEditor --mcp
```

引擎启动后自动监听 stdin/stdout。Agent 连接后正常通信。

### TCP 模式（独立进程 / 远程调试）

```bash
# 引擎作为独立窗口运行，Agent 远程连接
./bin/PrismaEditor --mcp --mcp-transport=tcp --mcp-port=3100
```

引擎重启后 Agent 自动重连。

### 禁用 MCP

```cmake
# CMake 配置时关闭
cmake --preset editor-windows-x64-debug -DPRISMA_ENABLE_MCP=OFF
```

## 命令行参数

| 参数 | 描述 | 默认值 |
|------|------|--------|
| `--mcp` | 启用 MCP 服务器 | ON (编译时启用) |
| `--mcp-transport=stdio\|tcp` | 传输方式 | stdio |
| `--mcp-port=<port>` | TCP 端口 (TCP 模式) | 3100 |

> 注: MCP 默认编译进编辑器。通过 `PRISMA_ENABLE_MCP=OFF` 完全禁用。

## 协议

### 标准 MCP 方法

| 方法 | 描述 |
|------|------|
| `initialize` | 握手，交换能力声明 |
| `tools/list` | 获取所有可用工具 |
| `tools/call` | 调用指定工具 |

### Prisma 扩展方法

| 方法 | 描述 |
|------|------|
| `mcp/get_state_hash` | 获取当前引擎状态根哈希 |
| `mcp/discover_all_tools` | 发现所有工具 (同 tools/list) |
| `mcp/discover_tools` | 按分类发现工具 |

### 握手能力声明

```json
// Client → Server
{
    "jsonrpc": "2.0",
    "method": "initialize",
    "params": {
        "clientInfo": {"name": "claude-code", "version": "1.0"},
        "capabilities": {},
        "maxTokensPerResponse": 2000
    }
}

// Server → Client
{
    "jsonrpc": "2.0",
    "result": {
        "protocolVersion": "2025-03-26",
        "capabilities": {
            "prisma_extensions": [
                "delta",           // 哈希增量追踪
                "field_filter",    // 字段过滤
                "paginate",        // 分页
                "value_omit",      // 默认值省略
                "session_cache"    // 会话缓存
            ]
        }
    }
}
```

## 工具清单

### scene — 场景操作

| 工具 | 方法 | 参数 | 描述 |
|------|------|------|------|
| 层级 | `scene_get_hierarchy` | `fields` | 获取场景实体树 |
| 详情 | `scene_get_entity` | `entity_id`, `fields` | 获取实体详情 |
| 创建 | `scene_create_entity` | `name`, `parent_id` | 创建实体 |
| 删除 | `scene_delete_entity` | `entity_id` | 删除实体 |

### ecs — 组件操作

| 工具 | 方法 | 参数 | 描述 |
|------|------|------|------|
| 列表 | `ecs_component_list` | `entity_id` | 列出实体所有组件类型 |
| 获取 | `ecs_component_get` | `entity_id`, `component_type` | 获取组件数据 |

### engine — 引擎

| 工具 | 方法 | 描述 |
|------|------|------|
| 状态 | `engine_get_status` | 引擎运行状态、场景信息、编译配置 |
| 哈希 | `mcp/get_state_hash` | 当前根状态哈希（增量入口） |
| 构建 | `engine_get_build_info` | 编译器、平台、构建类型 |

### debug — 调试

| 工具 | 方法 | 描述 |
|------|------|------|
| 帧数据 | `debug_frame_stats` | FPS、Draw Calls、三角面数 |
| 日志 | `debug_log_get` | 引擎日志（支持级别过滤和分页） |

### asset — 资源

| 工具 | 方法 | 描述 |
|------|------|------|
| 列表 | `asset_list` | 按类型过滤、分页 |
| 详情 | `asset_get_info` | 资源的元数据 |

### editor — 编辑器

| 工具 | 方法 | 描述 |
|------|------|------|
| 选区 | `editor_get_selection` | 当前选中的实体 |
| 控制台 | `editor_console_get` | 编辑器控制台输出 |

### game — 运行时

| 工具 | 方法 | 描述 |
|------|------|------|
| 状态 | `game_get_state` | 游戏运行状态 |
| 推进 | `game_simulate` | 推进 N 帧 |

## Token 优化指南

### 字段过滤

只请求需要的字段，避免全量返回：

```json
// ❌ 不要 — 返回所有字段
{"method": "scene_get_entity", "params": {"entity_id": 42}}

// ✅ 推荐 — 只请求需要的 2 个字段
{"method": "scene_get_entity", "params": {"entity_id": 42, "fields": ["name", "position"]}}
```

### 增量查询

记录上次的 root_hash，请求差异：

```
// 第一步: 获取当前哈希
→ {"method": "mcp/get_state_hash"}
← {"root_hash": "0xA3F7C21D"}

// Agent 保存 hash 到会话变量

// 第二步: 状态变化后，重新获取
→ {"method": "scene_get_hierarchy", "params": {"_known_hash": "0xA3F7C21D"}}
← {"_delta": true, "changed_count": 3, ...}
```

### 分页

```json
{"method": "asset_list", "params": {"type": "texture", "page": 1, "per_page": 20}}
```

### Agent 工作流建议

```
1. mcp/discover_all_tools           → 了解引擎能力 (~1 次)
2. mcp/get_state_hash               → 获取初始哈希
3. scene_get_hierarchy               → 了解场景 (~1 次)
4. scene_get_entity(id, fields)      → 按需查询实体
5. ecs_component_get(entity, type)   → 按需查组件
6. [修改代码 → 编译 → 重启引擎]
7. mcp/get_state_hash                → 新哈希
8. scene_get_hierarchy(_known_hash)  → 增量差异
```

## 扩展指南 (注册自定义工具)

### C++ 中注册

```cpp
#include "mcp/MCPTool.h"
#include "mcp/MCPSubSystem.h"

class MyCustomTool : public Prisma::MCP::MCPTool {
public:
    std::string_view GetName() const override { return "my_custom_tool"; }
    std::string_view GetDescription() const override {
        return "Description shown to the AI agent.";
    }
    std::string_view GetCategory() const override { return "custom"; }
    
    nlohmann::json GetInputSchema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"param1", {{"type", "string"}}}
            }},
            {"required", {"param1"}}
        };
    }
    
    nlohmann::json Execute(const nlohmann::json& args) override {
        auto param1 = args["param1"].get<std::string>();
        // ... your logic ...
        return {{"result", "success"}};
    }
};

// 在应用初始化时注册:
auto* mcp = Engine::Get().GetSystem<MCP::MCPSubSystem>();
if (mcp) {
    mcp->RegisterTool<MyCustomTool>();
}
```

## 架构

```
Engine
└── MCPSubSystem (ISubSystem)
    ├── MCPServer (JSON-RPC 路由)
    │   ├── Transport (stdio | tcp)
    │   ├── Session (哈希增量 + Token 预算)
    │   └── ToolRegistry
    │       ├── SceneTools
    │       ├── ECSTools
    │       ├── EngineTools
    │       ├── DebugTools
    │       ├── AssetTools
    │       ├── EditorTools
    │       └── GameTools
    └── DeltaTracker (xxh3 哈希树)
```

### 条件编译

MCP 通过 `PRISMA_ENABLE_MCP` 编译选项控制：

```cmake
option(PRISMA_ENABLE_MCP "Enable MCP server for AI agent support" ON)
```

编辑器代码中通过 `#if defined(PRISMA_ENABLE_MCP)` 保护（Editor 域）。

## 已知限制

- TCP 传输: 目前只接受单个客户端连接
- ECS 组件: `ecs_component_get` 返回的数据格式依赖具体组件实现
- `ComponentSerializer`: 默认值省略逻辑预留框架，待具体组件类型实现
- 调试工具: `debug_frame_stats` 暂未接入引擎实际计数器
- Asset 工具: `asset_list` 暂未接入 AssetDatabase
