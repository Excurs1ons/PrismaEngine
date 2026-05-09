---
name: prisma-mcp
description: Use when working with Prisma Engine's built-in MCP (Model Context Protocol) server. AI agents query engine state (scene hierarchy, entities, components), execute tools, and get incremental state deltas. Supports stdio (subprocess) and TCP (remote) transports.
---

# Prisma Engine MCP — AI Agent Skill

## Quick Start

Prisma Engine has a built-in MCP server. AI editors (Claude Code, Cursor) connect via stdio or TCP to query and control engine state.

### Connect

```bash
# Stdio mode (subprocess — simple, no setup)
./bin/PrismaEditor --mcp

# TCP mode (independent process — engine stays running)
./bin/PrismaEditor --mcp --mcp-transport=tcp --mcp-port=3100
```

### Minimal conversation (handshake + tool discovery)

```
→ {"jsonrpc":"2.0","id":0,"method":"initialize",
     "params":{"clientInfo":{"name":"agent","version":"1.0"},"maxTokensPerResponse":2000}}
← {"jsonrpc":"2.0","id":0,"result":{
     "protocolVersion":"2025-03-26",
     "capabilities":{"prisma_extensions":["delta","field_filter","paginate","value_omit","session_cache"]}}}

→ {"jsonrpc":"2.0","id":1,"method":"tools/list"}
← {"jsonrpc":"2.0","id":1,"result":{"tools":[...]}}
```

## Token-Saving Workflow

```python
# 1. discover tools once, cache the list
→ tools/list → cache tool schemas

# 2. get initial state hash
→ mcp/get_state_hash → save root_hash

# 3. query first scene (full response)
→ scene_get_hierarchy

# 4. code change → rebuild → restart engine
→ mcp/get_state_hash → compare hashes

# 5. if hash changed: query with previous hash (gets delta only)
→ scene_get_hierarchy? {"_known_hash": "0x..."} → incremental
```

## Available Tools

### scene
| Tool | Purpose | Key Fields |
|------|---------|------------|
| `scene_get_hierarchy` | Scene entity tree | `fields` |
| `scene_get_entity` | Single entity details | `entity_id`, `fields` |
| `scene_create_entity` | Create an entity | `name`, `parent_id` |
| `scene_delete_entity` | Remove an entity | `entity_id` |

### ecs
| Tool | Purpose | Key Fields |
|------|---------|------------|
| `ecs_component_list` | Component types on entity | `entity_id` |
| `ecs_component_get` | Component data | `entity_id`, `component_type` |

### engine
| Tool | Purpose |
|------|---------|
| `engine_get_status` | Running state, FPS, scene info |
| `engine_get_build_info` | Compiler, platform, build config |
| `mcp/get_state_hash` | Root state hash for delta tracking |

### debug
| Tool | Purpose |
|------|---------|
| `debug_frame_stats` | FPS, draw calls, triangle count |
| `debug_log_get` | Engine log entries with filters |

### asset / editor / game
| Tool | Purpose |
|------|---------|
| `asset_list` | List assets by type (paginated) |
| `asset_get_info` | Asset metadata |
| `editor_get_selection` | Currently selected entity |
| `editor_console_get` | Editor console output |
| `game_get_state` | Game runtime state |

## Field Filtering (Save Tokens)

```json
// ❌ Returns ALL fields
{"method":"scene_get_entity","params":{"entity_id":42}}

// ✅ Returns only 2 fields
{"method":"scene_get_entity","params":{"entity_id":42,"fields":["name","position"]}}
```

## Delta Caching

Every response includes state tracking. The agent should:

1. Save `root_hash` from `mcp/get_state_hash` after each operation
2. Include `_known_hash` in repeat queries
3. Server responds with `{"_delta":true, "changed_count":N}` instead of full data

When change ratio exceeds 30%, server auto-switches to full snapshot.

## Architecture

```
Engine
└── MCPSubSystem (ISubSystem)
    ├── MCPServer
    │   ├── Transport (stdio | tcp)
    │   ├── ToolRegistry
    │   └── Session (delta tracker + token budget)
    ├── DeltaTracker (xxh3 hash tree)
    └── Tools (scene, ecs, engine, debug, asset, editor, game)
```

## Extension — Register Custom Tools

```cpp
class MyTool : public Prisma::MCP::MCPTool {
    std::string_view GetName() const override;
    std::string_view GetDescription() const override;
    std::string_view GetCategory() const override;
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
};

// In app initialization:
auto* mcp = engine.GetSystem<MCP::MCPSubSystem>();
if (mcp) mcp->RegisterTool<MyTool>();
```

## Compile-Run-Cycle (Agent Loop)

```
1. Query state      → mcp/get_state_hash → save hash
2. Modify code      → Agent edits source files
3. Build            → cmake --build --preset editor-...
4. Start engine     → ./PrismaEditor --mcp
5. Auto-reconnect   → Engine starts MCP server
6. Sync state       → scene_get_hierarchy(_known_hash) → delta
7. Query → edit → loop
```

## Common Mistakes

- **Not using `_known_hash`** — always cache and send it; reduces token cost significantly
- **Not filtering `fields`** — request only what you need; JSON response grows linearly with fields
- **Sending notifications with "id"** — notifications must NOT have "id" field, or server treats them as requests and returns MethodNotFound error
- **Disconnecting on engine restart** — engine restarts MCP server; just reconnect (TCP keeps connection, stdio requires subprocess restart)
- **Tool call with `_known_hash` in args** — include `_known_hash` at top level of `arguments`, not in request params directly
