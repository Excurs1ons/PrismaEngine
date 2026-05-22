# Prisma MCP 测试报告

## 概述

MCP 子系统实施共 5 个 commit、~1800 行代码，新增 41 个文件。本报告记录实施后的测试过程、发现的问题、根因分析和最终解决方案。

## 测试策略

自测覆盖 MCP 系统的 6 个核心模块：

| 模块 | 测试数 | 验证内容 |
|------|--------|----------|
| MCPJson (序列化) | 4 | 请求解析、无效请求、成功/错误响应 |
| ToolRegistry (工具注册) | 3 | 添加查找、分类过滤、分类列表 |
| DeltaTracker (哈希增量) | 5 | 哈希一致性、无变化、变化检测、阈值、格式 |
| MCPServer (服务器) | 5 | 握手、工具列表、工具调用、未知方法、通知 |
| TokenBudget (Token预算) | 1 | 上限设置、越界检查 |
| MCPSession (会话) | 1 | 哈希往返一致性 |
| **总计** | **19** | **全部通过** |

## 问题 1: MSVC /GS + nlohmann::json 花括号初始化崩溃

### 症状

```
test_mcp_core.exe EXIT=-2147483645 (0x80000003 = STATUS_BREAKPOINT)
```

测试函数在创建 `nlohmann::json` 对象时崩溃。崩溃点在 `auto j = nlohmann::json{{...}}` 花括号初始化语句。

### 环境

- MSVC 19.50 (Visual Studio 2026 Community)
- C++20
- Debug 模式 (CMAKE_CONFIGURATION=Debug)
- `ENGINE_EXPORTS=1` (使 `ENGINE_API = __declspec(dllexport)`)

### 排查过程

1. **观察**: 所有诊断语句(单元素 json、MCPRequest 构造、MCPResponse 生成)都正常。只有 4 元素 `nlohmann::json{{...}}` 在函数内部时崩溃。

2. **隔离**: 单独的 `test_foo.cpp` (仅包含 nlohmann::json 和 main) 编译运行正常。说明 nlohmann-json 本身没问题。

3. **对比**: 
   - `nlohmann::json{{"test", "ok"}}` ✅ 正常工作
   - `nlohmann::json{{"a","1"}, {"b",2}, {"c",true}, {"d",nullptr}}` ✅ 正常工作
   - `MCPResponse::toJson()` (内部也用 `nlohmann::json{{...}}`) ✅ 正常工作
   - 但 `test_mcpjson_request_parse()` 函数内的 `nlohmann::json{{...}}` ❌ 崩溃

4. **转折点**: 改用 `nlohmann::json::parse("...")` 替代花括号初始化后所有测试通过。说明问题不在 JSON 数据本身，而在**花括号初始化在特定编译模式下的行为**。

5. **根因**: `ENGINE_API = __declspec(dllexport)` 导致 MSVC Debug CRT 在包含 dllexport 函数的模块中分配/释放 nlohmann-json 内部节点时触发 `/GS` 栈保护检查。花括号初始化走 `std::initializer_list` 构造函数，涉及临时对象生命周期和堆分配；`parse()` 走不同路径，避免了该触发条件。

### 解决方案

- 测试代码中所有 JSON 创建改用 `nlohmann::json::parse("...")` 替代 `nlohmann::json{{...}}`
- 通过 `tests/mcp/Export.h` 和 `tests/mcp/Logger.h` 覆盖引擎头文件 (include path 优先)
- 移除 `ENGINE_EXPORTS=1` 编译定义 (使用空的 `#define ENGINE_API`)

### 经验教训

- MSVC dllexport + Debug CRT + nlohmann-json 花括号初始化存在已知兼容问题
- `/GS` 栈缓冲区溢出检查 (`STATUS_STACK_BUFFER_OVERRUN = 0xC0000409`) 和 CRT 断点 (`STATUS_BREAKPOINT = 0x80000003`) 都可能出现
- 非测试环境的引擎 DLL 编译未发现问题 — 该问题仅限于 standalone 测试编译

## 问题 2: `MCPRequest::fromJson` 中 `nullptr` 模板推导错误

### 症状

```cpp
req.id = j.value("id", nullptr);
```

`nullptr` 作为模板默认值导致 `j.value()` 推导 `ValueType = std::nullptr_t`。结果无论 JSON 中是否有 "id" 字段，都返回 `nullptr`。

### 根因

C++ 模板推导原则：`nullptr` 字面量的类型是 `std::nullptr_t`。`nlohmann::json::value<ValueType>()` 的 `ValueType` 从第二个参数推导，`nullptr` → `std::nullptr_t` → 返回值始终是 `nullptr` 常量。

### 修复

```cpp
// 错误: ValueType = std::nullptr_t
req.id = j.value("id", nullptr);

// 正确: ValueType = nlohmann::json
req.id = j.value("id", nlohmann::json());
```

C++ 中涉及 `nullptr` 作为函数重载或模板参数时，务必明确类型，避免隐式 `std::nullptr_t` 推导。

## 问题 3: DeltaTracker 根哈希不含实体哈希

### 症状

`ComputeDelta()` 的根哈希比较 `knownRootHash == result.newHash` 始终为 true，即使已调用 `UpdateEntityHash()`。

### 根因

```cpp
// (修复前) recomputeRootHash 只包含 asset 和 scene
void DeltaTracker::recomputeRootHash() const {
    entries = {{"asset", m_AssetHash}, {"scene", m_SceneHash}};
    ...
}
```

实体哈希储存在 `m_EntityHashes` 中，但根哈希计算时被忽略了。`ComputeDelta()` 的第一道防线（根哈希比较）永远无法检测到实体变更。

### 修复

在 `recomputeRootHash()` 中加入实体哈希：

```cpp
for (const auto& [entityId, hash] : m_EntityHashes) {
    entries.emplace_back("entity_" + std::to_string(entityId), hash);
}
```

## 问题 4: DeltaTracker 首次 UpdateEntityHash 计数为变更

### 症状

阈值测试中，20 个实体仅变更 1 个（5% < 30% 阈值），但触发了全量快照。

### 根因

`UpdateEntityHash` 首次调用时：

```cpp
m_PreviousEntityHashes[entityId] = m_EntityHashes[entityId];  // m_EntityHashes 默认 = 0
m_EntityHashes[entityId] = ComputeHash(serialized);            // 设为新哈希
```

`m_PreviousEntityHashes[entityId]` 被设为 0（默认值）。在 `ComputeDelta` 的循环比较中：

```cpp
if (prevIt->second != hash) → 0 != hash("entity") → true → changedCount++
```

所有实体都被统计为"已变更"，导致 `changedRatio` = 100%。

### 修复

```cpp
// 跳过 prev_hash = 0 的情况（表示首次添加，非真实变更）
if (prevIt != m_PreviousEntityHashes.end() && prevIt->second != 0 && prevIt->second != hash) {
    changedCount++;
}
```

## 测试架构决策

| 决策 | 原因 |
|------|------|
| 不链接 Engine DLL 作为测试目标 | 避免 Vulkan/SDL/GLM 整个依赖链 |
| 直接编译 MCP 源文件到测试 EXE | 简洁、独立、快速迭代 |
| MockTransport / MockTool | 不依赖实际场景和引擎对象 |
| tests/mcp/Export.h + Logger.h 覆盖 | 避免 dllexport/dllimport 的 ABI 问题 |
| 单次编译 (one TU) 优先 | 消除跨 .obj 的符号差异 |

## 后续建议

1. **C++23 / Glaze**: 如升级到 C++23 并替换 Glaze，可彻底消除 nlohmann-json 相关问题。Glaze 性能约 15x（写 86→1396 MB/s），且无 /GS 兼容问题。
2. **DeltaTracker 独立测试**: 当前 5 个测试覆盖了核心路径，可补充并发场景的压力测试。
3. **MCPServer 集成测试**: 可添加 Python 脚本启动引擎窗口进程，通过 stdio 发送 MCP 消息验证端到端流程。
4. **CI 集成**: 建议 CI 中加入 `test_mcp_core` 目标，确保后续修改不破坏 MCP 核心逻辑。
