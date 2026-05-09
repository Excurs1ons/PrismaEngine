# Prisma MCP 支持实现计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 为 Prisma Engine 实现内置 MCP (Model Context Protocol) 支持，使 AI Agent 能通过标准协议实时查询和控制引擎状态。

**Architecture:** 以 `ISubSystem` 方式注册到引擎生命周期中。核心层：传输层抽象（stdio/TCP）→ JSON-RPC 消息路由 → 工具注册表 → Session 管理（内容哈希增量追踪）。工具分三层注册：引擎核心层 → 编辑器层 → 游戏层。

**Tech Stack:** C++20, nlohmann-json (已有), xxhash (新增), OS socket API (Winsock2/POSIX)

---

## Phase 1: MCP 协议核心 + 传输层

### Task 1.1: 创建 MCP 目录结构和 CMake 集成

**Files:**
- Create: `src/engine/mcp/CMakeLists.txt`
- Modify: `src/engine/CMakeLists.txt` (include MCP sources)
- Modify: `cmake/DependencyVersions.cmake` (add xxhash version)
- Modify: `cmake/FetchThirdPartyDeps.cmake` (add xxhash fetch)
- Modify: `cmake/DeviceOptions.cmake` (add PRISMA_ENABLE_MCP option)

**Step 1: Define xxhash version and fetch**

In `cmake/DependencyVersions.cmake`, add at end:
```cmake
set(PRISMA_DEP_XXHASH_VERSION "v0.8.2")
```

In `cmake/FetchThirdPartyDeps.cmake`, add after other Prisma_Declare_Dependency lines:
```cmake
Prisma_Declare_Dependency(xxhash https://github.com/Cyan4973/xxHash.git ${PRISMA_DEP_XXHASH_VERSION})
```

**Step 2: Add compile option**

At end of `cmake/DeviceOptions.cmake` (before final message block):
```cmake
option(PRISMA_ENABLE_MCP "Enable MCP server for AI agent support" ON)
```

Add to the configuration output section:
```cmake
message(STATUS "MCP:")
if(PRISMA_ENABLE_MCP)
    message(STATUS "  - MCP Server (Agent protocol)")
endif()
```

**Step 3: Create MCP CMakeLists.txt**

Create `src/engine/mcp/CMakeLists.txt`:
```cmake
if(PRISMA_ENABLE_MCP)
    # xxhash will be linked via Engine's target_link_libraries
    
    # Collect sources
    set(MCP_SOURCES
        # Transport
        mcp/transport/TransportStdio.cpp
        mcp/transport/TransportTCP.cpp
        # Protocol core
        mcp/MCPServer.cpp
        mcp/MCPTool.cpp
        mcp/MCPSubSystem.cpp
        # Session
        mcp/MCPSession.cpp
        mcp/session/DeltaTracker.cpp
        mcp/session/TokenBudget.cpp
        # Serialization
        mcp/serialization/ComponentSerializer.cpp
        mcp/serialization/MCPJson.cpp
        # Tools
        mcp/tools/SceneTools.cpp
        mcp/tools/ECSTools.cpp
        mcp/tools/EngineTools.cpp
        mcp/tools/DebugTools.cpp
        mcp/tools/AssetTools.cpp
        mcp/tools/EditorTools.cpp
        mcp/tools/GameTools.cpp
    )
    
    set(MCP_HEADERS
        mcp/transport/Transport.h
        mcp/transport/TransportStdio.h
        mcp/transport/TransportTCP.h
        mcp/MCPServer.h
        mcp/MCPTool.h
        mcp/MCPSubSystem.h
        mcp/MCPSession.h
        mcp/session/DeltaTracker.h
        mcp/session/TokenBudget.h
        mcp/serialization/ComponentSerializer.h
        mcp/serialization/MCPJson.h
        mcp/tools/SceneTools.h
        mcp/tools/ECSTools.h
        mcp/tools/EngineTools.h
        mcp/tools/DebugTools.h
        mcp/tools/AssetTools.h
        mcp/tools/EditorTools.h
        mcp/tools/GameTools.h
    )
    
    # These are appended to Engine's target_sources in the parent CMakeLists.txt
endif()
```

**Step 4: Modify engine CMakeLists.txt**

In `src/engine/CMakeLists.txt`, after existing source collections, add:
```cmake
# ========== MCP Server ==========
include(mcp/CMakeLists.txt)

if(PRISMA_ENABLE_MCP)
    list(APPEND CORE_SOURCES ${MCP_SOURCES})
    list(APPEND CORE_HEADERS ${MCP_HEADERS})
endif()
```

After `target_sources(Engine PRIVATE ${CORE_SOURCES} ${CORE_HEADERS})`, add:
```cmake
if(PRISMA_ENABLE_MCP)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_MCP)
    FetchContent_MakeAvailable(xxhash nlohmann_json)
    target_link_libraries(Engine PRIVATE xxhash::xxhash)
endif()
```

**Step 5: Create empty source/header files for all listed files**

```bash
mkdir -p src/engine/mcp/transport src/engine/mcp/session src/engine/mcp/serialization src/engine/mcp/tools
touch src/engine/mcp/transport/Transport.h
touch src/engine/mcp/transport/TransportStdio.h
touch src/engine/mcp/transport/TransportStdio.cpp
touch src/engine/mcp/transport/TransportTCP.h
touch src/engine/mcp/transport/TransportTCP.cpp
touch src/engine/mcp/MCPServer.h
touch src/engine/mcp/MCPServer.cpp
touch src/engine/mcp/MCPTool.h
touch src/engine/mcp/MCPTool.cpp
touch src/engine/mcp/MCPSubSystem.h
touch src/engine/mcp/MCPSubSystem.cpp
touch src/engine/mcp/MCPSession.h
touch src/engine/mcp/MCPSession.cpp
touch src/engine/mcp/session/DeltaTracker.h
touch src/engine/mcp/session/DeltaTracker.cpp
touch src/engine/mcp/session/TokenBudget.h
touch src/engine/mcp/session/TokenBudget.cpp
touch src/engine/mcp/serialization/ComponentSerializer.h
touch src/engine/mcp/serialization/ComponentSerializer.cpp
touch src/engine/mcp/serialization/MCPJson.h
touch src/engine/mcp/serialization/MCPJson.cpp
touch src/engine/mcp/tools/SceneTools.h
touch src/engine/mcp/tools/SceneTools.cpp
touch src/engine/mcp/tools/ECSTools.h
touch src/engine/mcp/tools/ECSTools.cpp
touch src/engine/mcp/tools/EngineTools.h
touch src/engine/mcp/tools/EngineTools.cpp
touch src/engine/mcp/tools/DebugTools.h
touch src/engine/mcp/tools/DebugTools.cpp
touch src/engine/mcp/tools/AssetTools.h
touch src/engine/mcp/tools/AssetTools.cpp
touch src/engine/mcp/tools/EditorTools.h
touch src/engine/mcp/tools/EditorTools.cpp
touch src/engine/mcp/tools/GameTools.h
touch src/engine/mcp/tools/GameTools.cpp
```

**Step 6: Verify build compiles (engine target)**

Run: `cmake --preset engine-windows-x64-debug`
Expected: Build succeeds. PRISMA_ENABLE_MCP is ON by default.

**Step 7: Commit**

```bash
git add cmake/DependencyVersions.cmake cmake/FetchThirdPartyDeps.cmake cmake/DeviceOptions.cmake src/engine/CMakeLists.txt src/engine/mcp/
git commit -m "feat(mcp): scaffold MCP subsystem directory structure and CMake integration"
```

---

### Task 1.2: Implement Transport abstraction

**Files:**
- Create: `src/engine/mcp/transport/Transport.h`
- Create: `src/engine/mcp/transport/TransportStdio.h`
- Create: `src/engine/mcp/transport/TransportStdio.cpp`

**Step 1: Write Transport.h**

```cpp
#pragma once
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace Prisma {
namespace MCP {

using MCPMessageHandler = std::function<void(const nlohmann::json& message)>;

class ENGINE_API Transport {
public:
    virtual ~Transport() = default;
    
    virtual bool Start(MCPMessageHandler handler) = 0;
    virtual void Stop() = 0;
    virtual bool Send(const nlohmann::json& message) = 0;
    virtual bool IsConnected() const = 0;
    virtual std::string_view GetName() const = 0;
};

} // namespace MCP
} // namespace Prisma
```

**Step 2: Write TransportStdio.h**

```cpp
#pragma once
#include "Transport.h"
#include <thread>
#include <atomic>

namespace Prisma {
namespace MCP {

class ENGINE_API TransportStdio : public Transport {
public:
    TransportStdio();
    ~TransportStdio() override;

    bool Start(MCPMessageHandler handler) override;
    void Stop() override;
    bool Send(const nlohmann::json& message) override;
    bool IsConnected() const override;
    std::string_view GetName() const override { return "stdio"; }

private:
    void readLoop();
    void processLine(const std::string& line);

    std::thread m_ReadThread;
    std::atomic<bool> m_Running{false};
    MCPMessageHandler m_Handler;
};

} // namespace MCP
} // namespace Prisma
```

**Step 3: Write TransportStdio.cpp**

```cpp
#include "TransportStdio.h"
#include <iostream>
#include <sstream>
#include <cstdio>

namespace Prisma {
namespace MCP {

TransportStdio::TransportStdio() = default;
TransportStdio::~TransportStdio() { Stop(); }

bool TransportStdio::Start(MCPMessageHandler handler) {
    if (m_Running) return false;
    m_Handler = std::move(handler);
    m_Running = true;
    m_ReadThread = std::thread(&TransportStdio::readLoop, this);
    return true;
}

void TransportStdio::Stop() {
    m_Running = false;
    if (m_ReadThread.joinable()) {
        m_ReadThread.join();
    }
}

bool TransportStdio::Send(const nlohmann::json& message) {
    if (!m_Running) return false;
    try {
        std::string output = message.dump() + "\n";
        std::cout << output << std::flush;
        return true;
    } catch (...) {
        return false;
    }
}

bool TransportStdio::IsConnected() const {
    return m_Running;
}

void TransportStdio::readLoop() {
    std::string line;
    while (m_Running && std::getline(std::cin, line)) {
        if (!m_Running) break;
        processLine(line);
    }
    m_Running = false;
}

void TransportStdio::processLine(const std::string& line) {
    if (line.empty()) return;
    try {
        auto json = nlohmann::json::parse(line);
        if (m_Handler) {
            m_Handler(json);
        }
    } catch (const nlohmann::json::parse_error& e) {
        // Send parse error as JSON-RPC error response
        nlohmann::json err = {
            {"jsonrpc", "2.0"},
            {"error", {{"code", -32700}, {"message", std::string("Parse error: ") + e.what()}}}
        };
        Send(err);
    }
}

} // namespace MCP
} // namespace Prisma
```

**Step 4: Write TransportTCP.h and TransportTCP.cpp (minimal stub)**

TransportTCP.h:
```cpp
#pragma once
#include "Transport.h"
#include <thread>
#include <atomic>

namespace Prisma {
namespace MCP {

class ENGINE_API TransportTCP : public Transport {
public:
    explicit TransportTCP(uint16_t port = 3100);
    ~TransportTCP() override;

    bool Start(MCPMessageHandler handler) override;
    void Stop() override;
    bool Send(const nlohmann::json& message) override;
    bool IsConnected() const override;
    std::string_view GetName() const override { return "tcp"; }

private:
    void acceptLoop();

    uint16_t m_Port;
    std::thread m_AcceptThread;
    std::atomic<bool> m_Running{false};
    MCPMessageHandler m_Handler;
    int m_ServerSocket = -1;
    int m_ClientSocket = -1;
};

} // namespace MCP
} // namespace Prisma
```

TransportTCP.cpp (skeleton - full implementation in later task):
```cpp
#include "TransportTCP.h"

namespace Prisma {
namespace MCP {

TransportTCP::TransportTCP(uint16_t port) : m_Port(port) {}
TransportTCP::~TransportTCP() { Stop(); }

bool TransportTCP::Start(MCPMessageHandler handler) {
    m_Handler = std::move(handler);
    // Full implementation uses Winsock2 or POSIX sockets
    // Placeholder for now - returns false (use stdio as default)
    return false;
}

void TransportTCP::Stop() { m_Running = false; }
bool TransportTCP::Send(const nlohmann::json&) { return false; }
bool TransportTCP::IsConnected() const { return false; }

} // namespace MCP
} // namespace Prisma
```

**Step 5: Build and verify**

Run: `cmake --build --preset engine-windows-x64-debug` (or just compile Engine target)
Expected: No compile errors.

**Step 6: Commit**

```bash
git add src/engine/mcp/transport/
git commit -m "feat(mcp): implement transport abstraction with stdio backend"
```

---

### Task 1.3: Implement JSON-RPC message handling (MCPServer)

**Files:**
- Create: `src/engine/mcp/MCPServer.h`
- Create: `src/engine/mcp/MCPServer.cpp`
- Create: `src/engine/mcp/serialization/MCPJson.h`
- Create: `src/engine/mcp/serialization/MCPJson.cpp`

**Step 1: Write MCPJson.h**

```cpp
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <optional>

namespace Prisma {
namespace MCP {

// MCP Protocol message types
struct MCPRequest {
    std::string jsonrpc = "2.0";
    std::string method;
    nlohmann::json params = nlohmann::json::object();
    nlohmann::json id; // int or string
    
    nlohmann::json toJson() const;
    static std::optional<MCPRequest> fromJson(const nlohmann::json& j);
};

struct MCPResponse {
    std::string jsonrpc = "2.0";
    nlohmann::json id = nullptr;
    nlohmann::json result = nlohmann::json::object();
    std::optional<nlohmann::json> error = std::nullopt;
    
    nlohmann::json toJson() const;
    static MCPResponse Success(nlohmann::json id, nlohmann::json result);
    static MCPResponse Error(nlohmann::json id, int code, const std::string& message, nlohmann::json data = nullptr);
};

// Standard JSON-RPC error codes
namespace ErrorCode {
    constexpr int ParseError = -32700;
    constexpr int InvalidRequest = -32600;
    constexpr int MethodNotFound = -32601;
    constexpr int InvalidParams = -32602;
    constexpr int InternalError = -32603;
}

} // namespace MCP
} // namespace Prisma
```

**Step 2: Write MCPJson.cpp**

```cpp
#include "MCPJson.h"

namespace Prisma {
namespace MCP {

nlohmann::json MCPRequest::toJson() const {
    nlohmann::json j = {
        {"jsonrpc", jsonrpc},
        {"method", method},
        {"params", params},
        {"id", id}
    };
    return j;
}

std::optional<MCPRequest> MCPRequest::fromJson(const nlohmann::json& j) {
    if (!j.contains("method") || !j["method"].is_string()) return std::nullopt;
    
    MCPRequest req;
    req.jsonrpc = j.value("jsonrpc", "2.0");
    req.method = j["method"].get<std::string>();
    req.params = j.value("params", nlohmann::json::object());
    req.id = j.value("id", nullptr);
    return req;
}

nlohmann::json MCPResponse::toJson() const {
    nlohmann::json j = {{"jsonrpc", jsonrpc}, {"id", id}};
    if (error) {
        j["error"] = *error;
    } else {
        j["result"] = result;
    }
    return j;
}

MCPResponse MCPResponse::Success(nlohmann::json id, nlohmann::json result) {
    MCPResponse r;
    r.id = std::move(id);
    r.result = std::move(result);
    return r;
}

MCPResponse MCPResponse::Error(nlohmann::json id, int code, const std::string& message, nlohmann::json data) {
    MCPResponse r;
    r.id = std::move(id);
    r.error = {{"code", code}, {"message", message}};
    if (!data.is_null()) (*r.error)["data"] = std::move(data);
    return r;
}

} // namespace MCP
} // namespace Prisma
```

**Step 3: Write MCPServer.h**

```cpp
#pragma once
#include "transport/Transport.h"
#include "MCPJson.h"
#include "MCPTool.h"
#include "MCPSession.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace Prisma {
namespace MCP {

class ENGINE_API MCPServer {
public:
    explicit MCPServer(std::unique_ptr<Transport> transport);
    ~MCPServer();

    bool Start(ToolRegistry* registry);
    void Stop();
    bool IsRunning() const { return m_Running; }
    
    void SetSession(std::shared_ptr<MCPSession> session) { m_Session = std::move(session); }
    MCPSession* GetSession() { return m_Session.get(); }

private:
    void onMessage(const nlohmann::json& msg);
    void handleRequest(const MCPRequest& req);
    void handleInitialize(const MCPRequest& req);
    void handleListTools(const MCPRequest& req);
    void handleCallTool(const MCPRequest& req);
    void handleGetStateHash(const MCPRequest& req);
    void sendResponse(const MCPResponse& resp);
    void sendEvent(const std::string& method, const nlohmann::json& params);

    std::unique_ptr<Transport> m_Transport;
    ToolRegistry* m_Registry = nullptr;
    std::shared_ptr<MCPSession> m_Session;
    std::atomic<bool> m_Running{false};
    
    // Notification handlers (non-blocking engine state pushes)
    std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> m_NotificationHandlers;
};

} // namespace MCP
} // namespace Prisma
```

**Step 4: Write MCPServer.cpp**

```cpp
#include "MCPServer.h"
#include "Logger.h"
#include <set>

namespace Prisma {
namespace MCP {

MCPServer::MCPServer(std::unique_ptr<Transport> transport)
    : m_Transport(std::move(transport)) {}

MCPServer::~MCPServer() { Stop(); }

bool MCPServer::Start(ToolRegistry* registry) {
    if (!m_Transport || !registry) return false;
    m_Registry = registry;
    m_Running = true;
    
    // Register standard MCP handlers
    m_NotificationHandlers["notifications/initialized"] = [](const nlohmann::json&) {};
    
    return m_Transport->Start([this](const nlohmann::json& msg) {
        onMessage(msg);
    });
}

void MCPServer::Stop() {
    m_Running = false;
    if (m_Transport) m_Transport->Stop();
}

void MCPServer::onMessage(const nlohmann::json& msg) {
    if (!msg.is_object()) return;
    
    // Handle notifications (no "id" field = no response expected)
    if (!msg.contains("id") || msg["id"].is_null()) {
        std::string method = msg.value("method", "");
        auto it = m_NotificationHandlers.find(method);
        if (it != m_NotificationHandlers.end()) {
            it->second(msg.value("params", nlohmann::json::object()));
        }
        return;
    }
    
    // Handle requests
    auto req = MCPRequest::fromJson(msg);
    if (!req) {
        sendResponse(MCPResponse::Error(nullptr, ErrorCode::InvalidRequest, "Invalid request"));
        return;
    }
    handleRequest(*req);
}

void MCPServer::handleRequest(const MCPRequest& req) {
    // Standard MCP methods
    if (req.method == "initialize") {
        handleInitialize(req);
    } else if (req.method == "notifications/initialized") {
        // No response needed for this notification, but if id present, send ack
        sendResponse(MCPResponse::Success(req.id, {{"ok", true}}));
    } else if (req.method == "tools/list") {
        handleListTools(req);
    } else if (req.method == "tools/call") {
        handleCallTool(req);
    } else if (req.method == "mcp/get_state_hash") {
        handleGetStateHash(req);
    } else if (req.method == "mcp/discover_all_tools") {
        handleListTools(req);
    } else if (req.method == "mcp/discover_tools") {
        handleListTools(req);
    } else {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::MethodNotFound,
            "Unknown method: " + req.method));
    }
}

void MCPServer::handleInitialize(const MCPRequest& req) {
    auto clientInfo = req.params.value("clientInfo", nlohmann::json::object());
    auto caps = req.params.value("capabilities", nlohmann::json::object());
    
    LOG_INFO("MCP", "Client connected: {} v{}",
        clientInfo.value("name", "unknown"),
        clientInfo.value("version", "?"));
    
    // Extract agent's token budget
    int maxTokens = req.params.value("maxTokensPerResponse", 2000);
    if (m_Session) {
        m_Session->SetTokenBudget(maxTokens);
    }
    
    // Prisma MCP extension capabilities
    nlohmann::json result = {
        {"protocolVersion", "2025-03-26"},
        {"capabilities", {
            {"tools", {{"listChanged", false}}},
            {"prisma_extensions", {
                "delta",           // Hash-based delta tracking
                "field_filter",    // Field-level response filtering
                "paginate",        // Pagination support
                "value_omit",      // Default value omission
                "session_cache"    // Session-aware caching
            }}
        }},
        {"serverInfo", {
            {"name", "PrismaEngine MCP"},
            {"version", "1.0.0"}
        }}
    };
    
    sendResponse(MCPResponse::Success(req.id, result));
}

void MCPServer::handleListTools(const MCPRequest& req) {
    if (!m_Registry) {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::InternalError, "Tool registry not available"));
        return;
    }
    
    auto filter = req.params.value("category", std::string());
    auto tools = m_Registry->GetToolList(filter);
    
    nlohmann::json toolsJson = nlohmann::json::array();
    for (const auto& tool : tools) {
        nlohmann::json t;
        t["name"] = tool->GetName();
        t["description"] = tool->GetDescription();
        t["inputSchema"] = tool->GetInputSchema();
        t["category"] = tool->GetCategory();
        toolsJson.push_back(std::move(t));
    }
    
    nlohmann::json result = {{"tools", toolsJson}};
    sendResponse(MCPResponse::Success(req.id, result));
}

void MCPServer::handleCallTool(const MCPRequest& req) {
    if (!m_Registry) {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::InternalError, "Tool registry not available"));
        return;
    }
    
    std::string toolName = req.params.value("name", "");
    auto arguments = req.params.value("arguments", nlohmann::json::object());
    
    // Extract session context
    std::string knownHash = arguments.value("_known_hash", std::string());
    if (!knownHash.empty() && m_Session) {
        auto delta = m_Session->TryGetDelta(knownHash, toolName, arguments);
        if (delta.is_object() && delta.contains("_delta")) {
            // Send delta response directly
            sendResponse(MCPResponse::Success(req.id, delta));
            return;
        }
    }
    
    auto* tool = m_Registry->FindTool(toolName);
    if (!tool) {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::MethodNotFound,
            "Tool not found: " + toolName));
        return;
    }
    
    try {
        auto result = tool->Execute(arguments);
        sendResponse(MCPResponse::Success(req.id, result));
    } catch (const std::exception& e) {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::InternalError,
            "Tool execution error: " + std::string(e.what())));
    }
}

void MCPServer::handleGetStateHash(const MCPRequest& req) {
    if (!m_Session) {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::InternalError, "Session not available"));
        return;
    }
    nlohmann::json result = {
        {"root_hash", m_Session->GetRootHashString()},
        {"version", m_Session->GetVersion()}
    };
    sendResponse(MCPResponse::Success(req.id, result));
}

void MCPServer::sendResponse(const MCPResponse& resp) {
    if (m_Transport && m_Transport->IsConnected()) {
        m_Transport->Send(resp.toJson());
    }
}

void MCPServer::sendEvent(const std::string& method, const nlohmann::json& params) {
    if (!m_Transport || !m_Transport->IsConnected()) return;
    nlohmann::json msg = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    };
    m_Transport->Send(msg);
}

} // namespace MCP
} // namespace Prisma
```

**Step 5: Build and verify**

Run: `cmake --build --preset engine-windows-x64-debug` (or compile Engine target)
Expected: No compile errors.

**Step 6: Commit**

```bash
git add src/engine/mcp/MCPServer.h src/engine/mcp/MCPServer.cpp src/engine/mcp/serialization/MCPJson.h src/engine/mcp/serialization/MCPJson.cpp
git commit -m "feat(mcp): implement JSON-RPC message handling and MCP server core"
```

---

## Phase 2: 工具系统 + 核心工具

### Task 2.1: Implement tool registration system (MCPTool)

**Files:**
- Modify: `src/engine/mcp/MCPTool.h`
- Modify: `src/engine/mcp/MCPTool.cpp`

**Step 1: Write MCPTool.h**

```cpp
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Export.h"

namespace Prisma {
namespace MCP {

class ENGINE_API MCPTool {
public:
    virtual ~MCPTool() = default;
    
    virtual std::string_view GetName() const = 0;
    virtual std::string_view GetDescription() const = 0;
    virtual std::string_view GetCategory() const = 0;
    virtual nlohmann::json GetInputSchema() const = 0;
    virtual nlohmann::json Execute(const nlohmann::json& args) = 0;
    
    // Optional: field-level filtering support
    virtual bool SupportsFieldFilter() const { return false; }
};

class ENGINE_API ToolRegistry {
public:
    void AddTool(std::unique_ptr<MCPTool> tool);
    void AddTools(std::vector<std::unique_ptr<MCPTool>> tools);
    
    MCPTool* FindTool(const std::string& name) const;
    std::vector<MCPTool*> GetToolList(const std::string& category = "") const;
    std::vector<std::string> GetCategories() const;
    size_t GetToolCount() const { return m_Tools.size(); }
    
private:
    std::unordered_map<std::string, std::unique_ptr<MCPTool>> m_Tools;
};

} // namespace MCP
} // namespace Prisma
```

**Step 2: Write MCPTool.cpp**

```cpp
#include "MCPTool.h"
#include <algorithm>

namespace Prisma {
namespace MCP {

void ToolRegistry::AddTool(std::unique_ptr<MCPTool> tool) {
    if (tool) {
        m_Tools[tool->GetName()] = std::move(tool);
    }
}

void ToolRegistry::AddTools(std::vector<std::unique_ptr<MCPTool>> tools) {
    for (auto& tool : tools) {
        AddTool(std::move(tool));
    }
}

MCPTool* ToolRegistry::FindTool(const std::string& name) const {
    auto it = m_Tools.find(name);
    return (it != m_Tools.end()) ? it->second.get() : nullptr;
}

std::vector<MCPTool*> ToolRegistry::GetToolList(const std::string& category) const {
    std::vector<MCPTool*> result;
    for (const auto& [name, tool] : m_Tools) {
        if (category.empty() || tool->GetCategory() == category) {
            result.push_back(tool.get());
        }
    }
    // Sort by name for deterministic output
    std::sort(result.begin(), result.end(), [](MCPTool* a, MCPTool* b) {
        return a->GetName() < b->GetName();
    });
    return result;
}

std::vector<std::string> ToolRegistry::GetCategories() const {
    std::unordered_set<std::string> cats;
    for (const auto& [name, tool] : m_Tools) {
        cats.insert(std::string(tool->GetCategory()));
    }
    std::vector<std::string> result(cats.begin(), cats.end());
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace MCP
} // namespace Prisma
```

**Step 3: Build and verify**

Run: `cmake --build --preset engine-windows-x64-debug`
Expected: No compile errors.

**Step 4: Commit**

```bash
git add src/engine/mcp/MCPTool.h src/engine/mcp/MCPTool.cpp
git commit -m "feat(mcp): implement tool registration system with category support"
```

---

### Task 2.2: Implement Scene tools

**Files:**
- Create: `src/engine/mcp/tools/SceneTools.h`
- Create: `src/engine/mcp/tools/SceneTools.cpp`

**Step 1: Write SceneTools.h**

```cpp
#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; class SceneManager; class Scene; }

namespace Prisma {
namespace MCP {

class SceneHierarchyTool : public MCPTool {
public:
    explicit SceneHierarchyTool(Engine* engine);
    
    std::string_view GetName() const override { return "scene_get_hierarchy"; }
    std::string_view GetDescription() const override {
        return "Get the scene entity hierarchy. Returns a tree of entities with names, IDs, and child counts. Supports field-level filtering.";
    }
    std::string_view GetCategory() const override { return "scene"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
    bool SupportsFieldFilter() const override { return true; }

private:
    Engine* m_Engine;
};

class SceneEntityTool : public MCPTool {
public:
    explicit SceneEntityTool(Engine* engine);
    
    std::string_view GetName() const override { return "scene_get_entity"; }
    std::string_view GetDescription() const override {
        return "Get detailed information about a scene entity. Supports field-level filtering.";
    }
    std::string_view GetCategory() const override { return "scene"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;
    bool SupportsFieldFilter() const override { return true; }

private:
    Engine* m_Engine;
};

class SceneCreateEntityTool : public MCPTool {
public:
    explicit SceneCreateEntityTool(Engine* engine);
    
    std::string_view GetName() const override { return "scene_create_entity"; }
    std::string_view GetDescription() const override { return "Create a new entity in the current scene."; }
    std::string_view GetCategory() const override { return "scene"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;

private:
    Engine* m_Engine;
};

class SceneDeleteEntityTool : public MCPTool {
public:
    explicit SceneDeleteEntityTool(Engine* engine);
    
    std::string_view GetName() const override { return "scene_delete_entity"; }
    std::string_view GetDescription() const override { return "Delete an entity from the current scene."; }
    std::string_view GetCategory() const override { return "scene"; }
    nlohmann::json GetInputSchema() const override;
    nlohmann::json Execute(const nlohmann::json& args) override;

private:
    Engine* m_Engine;
};

} // namespace MCP
} // namespace Prisma
```

**Step 2: Write SceneTools.cpp**

```cpp
#include "SceneTools.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "scene/GameObject.h"
#include "Logger.h"

namespace Prisma {
namespace MCP {

// ---- SceneHierarchyTool ----

SceneHierarchyTool::SceneHierarchyTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneHierarchyTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"fields", {{"type", "array", "items", {{"type", "string"}}}}}
        }}
    };
}

nlohmann::json SceneHierarchyTool::Execute(const nlohmann::json& args) {
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager->GetCurrentScene();
    if (!scene) {
        return {{"error", "No active scene"}, {"entities", nlohmann::json::array()}};
    }
    
    auto fields = args.value("fields", std::vector<std::string>{"name"});
    auto allEntities = scene->GetAllEntities();
    
    nlohmann::json entities = nlohmann::json::array();
    for (const auto& entity : allEntities) {
        nlohmann::json e;
        e["id"] = entity->GetID();
        
        if (fields.empty() || std::find(fields.begin(), fields.end(), "name") != fields.end())
            e["name"] = entity->GetName();
        
        entities.push_back(std::move(e));
    }
    
    return {{"scene_name", scene->GetName()}, {"entity_count", allEntities.size()}, {"entities", entities}};
}

// ---- SceneEntityTool ----

SceneEntityTool::SceneEntityTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_id", {{"type", "integer"}, {"description", "Entity ID to query"}}},
            {"fields", {{"type", "array", "items", {{"type", "string"}}}}}
        }},
        {"required", {"entity_id"}}
    };
}

nlohmann::json SceneEntityTool::Execute(const nlohmann::json& args) {
    auto entityId = args["entity_id"].get<uint32_t>();
    
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager->GetCurrentScene();
    if (!scene) return {{"error", "No active scene"}};
    
    auto entity = scene->FindEntity(entityId);
    if (!entity) return {{"error", "Entity not found"}, {"entity_id", entityId}};
    
    auto fields = args.value("fields", std::vector<std::string>{});
    auto fieldSet = std::set<std::string>(fields.begin(), fields.end());
    bool allFields = fields.empty();
    
    nlohmann::json result;
    result["entity_id"] = entityId;
    
    if (allFields || fieldSet.count("name"))
        result["name"] = entity->GetName();
    if (allFields || fieldSet.count("position"))
        result["position"] = {entity->GetPosition().x, entity->GetPosition().y, entity->GetPosition().z};
    if (allFields || fieldSet.count("parent_id"))
        result["parent_id"] = entity->GetParentID();
    
    return result;
}

// ---- SceneCreateEntityTool ----

SceneCreateEntityTool::SceneCreateEntityTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneCreateEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"name", {{"type", "string"}}},
            {"parent_id", {{"type", "integer"}}}
        }},
        {"required", {"name"}}
    };
}

nlohmann::json SceneCreateEntityTool::Execute(const nlohmann::json& args) {
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager->GetCurrentScene();
    if (!scene) return {{"error", "No active scene"}};
    
    auto name = args["name"].get<std::string>();
    // Use the engine's entity creation path
    auto entity = scene->CreateEntity(name);
    
    if (args.contains("parent_id")) {
        auto parentId = args["parent_id"].get<uint32_t>();
        auto parent = scene->FindEntity(parentId);
        if (parent) {
            parent->AddChild(entity);
        }
    }
    
    return {{"entity_id", entity->GetID()}, {"name", name}};
}

// ---- SceneDeleteEntityTool ----

SceneDeleteEntityTool::SceneDeleteEntityTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json SceneDeleteEntityTool::GetInputSchema() const {
    return {
        {"type", "object"},
        {"properties", {
            {"entity_id", {{"type", "integer"}, {"description", "Entity ID to delete"}}}
        }},
        {"required", {"entity_id"}}
    };
}

nlohmann::json SceneDeleteEntityTool::Execute(const nlohmann::json& args) {
    auto entityId = args["entity_id"].get<uint32_t>();
    
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager->GetCurrentScene();
    if (!scene) return {{"error", "No active scene"}};
    
    bool deleted = scene->DeleteEntity(entityId);
    return {{"deleted", deleted}, {"entity_id", entityId}};
}

} // namespace MCP
} // namespace Prisma
```

**Step 3: Build and verify**

Run: `cmake --build --preset engine-windows-x64-debug`
Expected: No compile errors.

**Step 4: Commit**

```bash
git add src/engine/mcp/tools/SceneTools.h src/engine/mcp/tools/SceneTools.cpp
git commit -m "feat(mcp): implement scene hierarchy and entity query tools"
```

---

### Task 2.3: Implement ECS tools

**Files:**
- Create: `src/engine/mcp/tools/ECSTools.h`
- Create: `src/engine/mcp/tools/ECSTools.cpp`

(Write ECS component list/get/set/add/remove tools, focusing on field-level component data access.)

**Step 1: Write ECSTools.h**

```cpp
#pragma once
#include "../MCPTool.h"

namespace Prisma { class Engine; }

namespace Prisma {
namespace MCP {

class ECSComponentListTool : public MCPTool { /* entity's component types */ };
class ECSComponentGetTool : public MCPTool { /* get component data */ };
class ECSComponentSetTool : public MCPTool { /* set component property */ };

} // namespace MCP
} // namespace Prisma
```

(Full implementations follow the same pattern as SceneTools — too long to inline here, but follow the exact same MCPTool contract.)

**Step 2: Build and commit**

```bash
git add src/engine/mcp/tools/ECSTools.h src/engine/mcp/tools/ECSTools.cpp
git commit -m "feat(mcp): implement ECS component query and mutation tools"
```

---

### Task 2.4: Implement Engine lifecycle tools

**Files:**
- Create: `src/engine/mcp/tools/EngineTools.h`
- Create: `src/engine/mcp/tools/EngineTools.cpp`

Tools:
- `engine_get_status` — running, fps, scene count
- `engine_get_state_hash` — returns current root hash for delta tracking
- `engine_get_build_info` — build configuration, git commit hash

**Step 1: Write and commit:**

```bash
git add src/engine/mcp/tools/EngineTools.h src/engine/mcp/tools/EngineTools.cpp
git commit -m "feat(mcp): implement engine status and build info tools"
```

---

### Task 2.5: Implement MCPSubSystem (ISubSystem integration)

**Files:**
- Create: `src/engine/mcp/MCPSubSystem.h`
- Create: `src/engine/mcp/MCPSubSystem.cpp`

**Step 1: Write MCPSubSystem.h**

```cpp
#pragma once
#include "core/ISubSystem.h"
#include "MCPServer.h"
#include "MCPTool.h"
#include "MCPSession.h"
#include <memory>
#include <vector>

namespace Prisma {
namespace MCP {

class ENGINE_API MCPSubSystem : public ISubSystem {
public:
    MCPSubSystem();
    ~MCPSubSystem() override;

    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

    // Tool registration
    template<typename T, typename... Args>
    T* RegisterTool(Args&&... args) {
        auto tool = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = tool.get();
        m_Registry.AddTool(std::move(tool));
        return ptr;
    }

    ToolRegistry& GetRegistry() { return m_Registry; }
    MCPServer* GetServer() { return m_Server.get(); }
    MCPSession* GetSession() { return m_Session.get(); }

    // Set transport before Initialize()
    void SetTransport(std::unique_ptr<Transport> transport);

private:
    std::unique_ptr<Transport> m_Transport;
    std::unique_ptr<MCPServer> m_Server;
    ToolRegistry m_Registry;
    std::shared_ptr<MCPSession> m_Session;
    uint32_t m_UpdateTick = 0;
};

} // namespace MCP
} // namespace Prisma
```

**Step 2: Write MCPSubSystem.cpp**

```cpp
#include "MCPSubSystem.h"
#include "transport/TransportStdio.h"
#include "Logger.h"

namespace Prisma {
namespace MCP {

MCPSubSystem::MCPSubSystem()
    : m_Transport(std::make_unique<TransportStdio>())
    , m_Session(std::make_shared<MCPSession>()) {}

MCPSubSystem::~MCPSubSystem() { Shutdown(); }

void MCPSubSystem::SetTransport(std::unique_ptr<Transport> transport) {
    if (transport) m_Transport = std::move(transport);
}

int MCPSubSystem::Initialize() {
    LOG_INFO("MCP", "Initializing MCP subsystem...");
    
    m_Server = std::make_unique<MCPServer>(std::move(m_Transport));
    m_Server->SetSession(m_Session);
    
    if (!m_Server->Start(&m_Registry)) {
        LOG_WARN("MCP", "MCP server failed to start (no client connected yet)");
        // Not a fatal error — engine works without MCP client
    }
    
    LOG_INFO("MCP", "MCP subsystem ready. Transport: {} | {} tools registered",
        m_Server->IsRunning() ? "connected" : "waiting",
        m_Registry.GetToolCount());
    
    return 0;
}

void MCPSubSystem::Shutdown() {
    if (m_Server) m_Server->Stop();
    LOG_INFO("MCP", "MCP subsystem shut down");
}

void MCPSubSystem::Update(Timestep ts) {
    m_UpdateTick++;
    // Periodically recompute state hashes (every 10 ticks)
    if (m_UpdateTick % 10 == 0 && m_Session) {
        m_Session->RefreshHashes();
    }
}

} // namespace MCP
} // namespace Prisma
```

**Step 3: Modify Engine.cpp to register MCPSubSystem and tools**

In `src/engine/app/Engine.cpp`, add includes at top:
```cpp
#if defined(PRISMA_ENABLE_MCP)
#include "mcp/MCPSubSystem.h"
#include "mcp/tools/SceneTools.h"
#include "mcp/tools/ECSTools.h"
#include "mcp/tools/EngineTools.h"
#endif
```

In `Engine::Initialize()`, after the existing system registrations, add:
```cpp
#if defined(PRISMA_ENABLE_MCP)
    LOG_INFO("Engine", "MCP subsystem is enabled");
    auto* mcp = AddSystem<MCP::MCPSubSystem>();
    
    // Register engine core tools
    mcp->RegisterTool<MCP::SceneHierarchyTool>(this);
    mcp->RegisterTool<MCP::SceneEntityTool>(this);
    mcp->RegisterTool<MCP::SceneCreateEntityTool>(this);
    mcp->RegisterTool<MCP::SceneDeleteEntityTool>(this);
    mcp->RegisterTool<MCP::ECSComponentListTool>(this);
    mcp->RegisterTool<MCP::ECSComponentGetTool>(this);
    mcp->RegisterTool<MCP::ECSComponentSetTool>(this);
    mcp->RegisterTool<MCP::EngineStatusTool>(this);
    mcp->RegisterTool<MCP::EngineStateHashTool>(this);
    mcp->RegisterTool<MCP::EngineBuildInfoTool>(this);
#endif
```

**Step 4: Build and verify**

Run: `cmake --build --preset engine-windows-x64-debug`
Expected: No compile errors. Engine links and runs.

**Step 5: Commit**

```bash
git add src/engine/mcp/MCPSubSystem.h src/engine/mcp/MCPSubSystem.cpp src/engine/app/Engine.cpp
git commit -m "feat(mcp): integrate MCP subsystem into engine lifecycle and register core tools"
```

---

## Phase 3: Session + Hash Delta Tracking

### Task 3.1: Implement DeltaTracker (xxh3_64 hash tree)

**Files:**
- Create: `src/engine/mcp/session/DeltaTracker.h`
- Create: `src/engine/mcp/session/DeltaTracker.cpp`
- Modify: `src/engine/mcp/MCPSession.h`
- Modify: `src/engine/mcp/MCPSession.cpp`

**Step 1: Write DeltaTracker.h**

```cpp
#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace Prisma {
namespace MCP {

using Hash64 = uint64_t;

inline Hash64 ComputeHash(std::string_view data) {
    return XXH3_64bits(data.data(), data.size());
}

struct DeltaResult {
    bool hasChanges = false;
    nlohmann::json delta;      // incremental changes
    nlohmann::json snapshot;   // full snapshot (when ratio > threshold)
    float changeRatio = 0.0f;
    Hash64 newHash = 0;
};

class DeltaTracker {
public:
    void UpdateEntityHash(uint32_t entityId, std::string_view serialized);
    void UpdateSceneHash(const std::string& entitiesJson);
    void UpdateAssetHash(const std::string& assetsJson);
    void RemoveEntityHash(uint32_t entityId);
    
    Hash64 GetEntityHash(uint32_t entityId) const;
    Hash64 GetSceneHash() const { return m_SceneHash; }
    Hash64 GetAssetHash() const { return m_AssetHash; }
    Hash64 GetRootHash() const;
    std::string GetRootHashString() const;
    
    // Delta computation
    DeltaResult ComputeDelta(Hash64 knownRootHash, const std::string& toolName, const nlohmann::json& args) const;
    
    // Threshold: if change ratio > this, return full snapshot instead of delta
    static constexpr float kDeltaThreshold = 0.3f;

private:
    // Per-entity hash tracking
    std::unordered_map<uint32_t, Hash64> m_EntityHashes;
    
    // Subsystem hashes
    Hash64 m_SceneHash = 0;
    Hash64 m_AssetHash = 0;
    
    // For tracking which entities are "new" vs "changed" vs "deleted"
    std::unordered_map<uint32_t, Hash64> m_PreviousEntityHashes;
    
    mutable bool m_RootHashDirty = true;
    mutable Hash64 m_CachedRootHash = 0;
    
    void recomputeRootHash() const;
};

} // namespace MCP
} // namespace Prisma
```

**Step 2: Write DeltaTracker.cpp**

```cpp
#include "DeltaTracker.h"
#include <algorithm>
#include <sstream>

namespace Prisma {
namespace MCP {

void DeltaTracker::UpdateEntityHash(uint32_t entityId, std::string_view serialized) {
    m_PreviousEntityHashes[entityId] = m_EntityHashes[entityId];
    m_EntityHashes[entityId] = ComputeHash(serialized);
    m_RootHashDirty = true;
}

void DeltaTracker::UpdateSceneHash(const std::string& entitiesJson) {
    m_SceneHash = ComputeHash(entitiesJson);
    m_RootHashDirty = true;
}

void DeltaTracker::UpdateAssetHash(const std::string& assetsJson) {
    m_AssetHash = ComputeHash(assetsJson);
    m_RootHashDirty = true;
}

void DeltaTracker::RemoveEntityHash(uint32_t entityId) {
    m_PreviousEntityHashes[entityId] = m_EntityHashes[entityId];
    m_EntityHashes.erase(entityId);
    m_RootHashDirty = true;
}

Hash64 DeltaTracker::GetEntityHash(uint32_t entityId) const {
    auto it = m_EntityHashes.find(entityId);
    return (it != m_EntityHashes.end()) ? it->second : 0;
}

Hash64 DeltaTracker::GetRootHash() const {
    if (m_RootHashDirty) recomputeRootHash();
    return m_CachedRootHash;
}

std::string DeltaTracker::GetRootHashString() const {
    auto h = GetRootHash();
    std::stringstream ss;
    ss << "0x" << std::hex << h;
    return ss.str();
}

void DeltaTracker::recomputeRootHash() const {
    // Stable concatenation: sort subsystem labels, then concat hash pairs
    std::vector<std::pair<std::string, Hash64>> entries = {
        {"scene", m_SceneHash},
        {"asset", m_AssetHash},
        {"root", 0}
    };
    std::sort(entries.begin(), entries.end());
    
    std::string combined;
    for (const auto& [label, hash] : entries) {
        combined += label;
        combined.append(reinterpret_cast<const char*>(&hash), sizeof(hash));
    }
    
    m_CachedRootHash = ComputeHash(combined);
    m_RootHashDirty = false;
}

DeltaResult DeltaTracker::ComputeDelta(Hash64 knownRootHash, const std::string& toolName, const nlohmann::json& args) const {
    DeltaResult result;
    result.newHash = GetRootHash();
    
    if (knownRootHash == result.newHash) {
        result.hasChanges = false;
        result.delta = {{"_unchanged", true}};
        return result;
    }
    
    // Count changes
    size_t changedCount = 0;
    size_t deletedCount = 0;
    size_t totalCount = std::max(m_EntityHashes.size(), m_PreviousEntityHashes.size());
    
    nlohmann::json changes = nlohmann::json::object();
    nlohmann::json deleted = nlohmann::json::array();
    
    for (const auto& [id, hash] : m_EntityHashes) {
        auto prevIt = m_PreviousEntityHashes.find(id);
        if (prevIt == m_PreviousEntityHashes.end() || prevIt->second != hash) {
            changedCount++;
        }
    }
    for (const auto& [id, hash] : m_PreviousEntityHashes) {
        if (m_EntityHashes.find(id) == m_EntityHashes.end()) {
            deletedCount++;
            deleted.push_back(id);
        }
    }
    
    result.changeRatio = (totalCount > 0) ? static_cast<float>(changedCount + deletedCount) / totalCount : 0.0f;
    
    if (result.changeRatio > kDeltaThreshold && totalCount > 5) {
        // Too many changes: return full snapshot trigger
        result.hasChanges = true;
        result.snapshot = {{"_full_snapshot", true}, {"reason", "change_ratio_exceeded"}, {"ratio", result.changeRatio}};
    } else {
        result.hasChanges = (changedCount > 0 || deletedCount > 0);
        if (result.hasChanges) {
            result.delta = {
                {"_delta_version", result.newHash},
                {"changed_count", changedCount},
                {"deleted_count", deletedCount}
            };
            if (deleted.size() > 0) result.delta["deleted_entity_ids"] = deleted;
        }
    }
    
    return result;
}

} // namespace MCP
} // namespace Prisma
```

**Step 3: Update MCPSession.h to include DeltaTracker**

```cpp
#pragma once
#include "session/DeltaTracker.h"
#include "session/TokenBudget.h"
#include <memory>
#include <atomic>

namespace Prisma {
namespace MCP {

class ENGINE_API MCPSession {
public:
    MCPSession();
    
    // Hash tracking
    DeltaTracker& GetDeltaTracker() { return m_DeltaTracker; }
    Hash64 GetRootHash() const { return m_DeltaTracker.GetRootHash(); }
    std::string GetRootHashString() const { return m_DeltaTracker.GetRootHashString(); }
    uint64_t GetVersion() const { return m_VersionCounter; }
    
    // Token budget
    void SetTokenBudget(int maxTokens) { m_TokenBudget.SetMaxTokens(maxTokens); }
    int GetTokenBudget() const { return m_TokenBudget.GetMaxTokens(); }
    bool WouldExceedBudget(const nlohmann::json& response) const;
    
    // Delta computation with budget awareness
    nlohmann::json TryGetDelta(const std::string& knownHashStr, const std::string& toolName, const nlohmann::json& args);
    
    // Periodic refresh
    void RefreshHashes();
    void MarkDirty() { m_VersionCounter++; }

private:
    DeltaTracker m_DeltaTracker;
    TokenBudget m_TokenBudget;
    std::atomic<uint64_t> m_VersionCounter{0};
};

} // namespace MCP
} // namespace Prisma
```

**Step 4: Write MCPSession.cpp**

```cpp
#include "MCPSession.h"
#include <sstream>

namespace Prisma {
namespace MCP {

MCPSession::MCPSession() = default;

nlohmann::json MCPSession::TryGetDelta(const std::string& knownHashStr, const std::string& toolName, const nlohmann::json& args) {
    // Parse known hash from hex string
    Hash64 knownHash = 0;
    if (knownHashStr.size() > 2 && knownHashStr.substr(0, 2) == "0x") {
        knownHash = std::stoull(knownHashStr, nullptr, 16);
    } else {
        knownHash = std::stoull(knownHashStr, nullptr, 16);
    }
    
    auto delta = m_DeltaTracker.ComputeDelta(knownHash, toolName, args);
    if (!delta.hasChanges) {
        return {{"_unchanged", true}};
    }
    
    if (!delta.snapshot.is_null()) {
        // Change ratio exceeded threshold
        return {{"_full_snapshot", true}, {"root_hash", GetRootHashString()}};
    }
    
    nlohmann::json result = delta.delta;
    result["root_hash"] = GetRootHashString();
    result["_delta"] = true;
    return result;
}

void MCPSession::RefreshHashes() {
    // Recompute root hash cache
    m_DeltaTracker.GetRootHash();
}

} // namespace MCP
} // namespace Prisma
```

**Step 5: Write TokenBudget.h**

```cpp
#pragma once
#include <cstdint>

namespace Prisma {
namespace MCP {

class TokenBudget {
public:
    void SetMaxTokens(int max) { m_MaxTokens = max; }
    int GetMaxTokens() const { return m_MaxTokens; }
    
    bool WouldExceed(int estimatedTokens) const {
        return estimatedTokens > m_MaxTokens;
    }

private:
    int m_MaxTokens = 2000;
};

} // namespace MCP
} // namespace Prisma
```

**Step 6: Build and verify**

Run: `cmake --build --preset engine-windows-x64-debug`
Expected: No compile errors.

**Step 7: Commit**

```bash
git add src/engine/mcp/session/ src/engine/mcp/MCPSession.h src/engine/mcp/MCPSession.cpp
git commit -m "feat(mcp): implement xxh3-based hash delta tracking and session management"
```

---

## Phase 4: 编辑器 + 调试工具

### Task 4.1: Editor tools

**Files:**
- Create: `src/engine/mcp/tools/EditorTools.h`
- Create: `src/engine/mcp/tools/EditorTools.cpp`
- Modify: `src/editor/core/Editor.h` (add EditorTool registration)
- Modify: `src/editor/core/Editor.cpp`

**Step 1: Write EditorTools.h/cpp** — tools: `editor_get_selection`, `editor_set_selection`, `editor_get_viewport_info`, `editor_console_get`

Tools access the Editor singleton via `Editor::Get()`.

**Step 2: Add tool registration in Editor::OnInitialize():**
```cpp
#if defined(PRISMA_ENABLE_MCP)
    auto* mcp = Engine::Get().GetSystem<MCP::MCPSubSystem>();
    if (mcp) {
        mcp->RegisterTool<MCP::EditorSelectionTool>(this);
        mcp->RegisterTool<MCP::EditorViewportTool>(this);
        mcp->RegisterTool<MCP::EditorConsoleTool>();
    }
#endif
```

**Step 3: Build and commit**

```bash
git add src/engine/mcp/tools/EditorTools.h src/engine/mcp/tools/EditorTools.cpp src/editor/core/Editor.h src/editor/core/Editor.cpp
git commit -m "feat(mcp): implement editor tools (selection, viewport, console)"
```

---

### Task 4.2: Debug tools

**Files:**
- Create: `src/engine/mcp/tools/DebugTools.h`
- Create: `src/engine/mcp/tools/DebugTools.cpp`

Tools:
- `debug_frame_stats` — FPS, draw calls, triangle count, VRAM usage
- `debug_log_get` — engine log with level filter + `since_tick` pagination
- `debug_gpu_resources` — GPU resource list (textures, buffers, pipelines with memory)

**Step 1: Write and commit**

```bash
git add src/engine/mcp/tools/DebugTools.h src/engine/mcp/tools/DebugTools.cpp
git commit -m "feat(mcp): implement debug and profiling tools (frame stats, log, GPU resources)"
```

---

## Phase 5: Asset + Game 工具 + 编译循环

### Task 5.1: Asset tools

**Files:**
- Create: `src/engine/mcp/tools/AssetTools.h`
- Create: `src/engine/mcp/tools/AssetTools.cpp`

Tools:
- `asset_list` — resource list with type filter and pagination
- `asset_get_info` — metadata (path, type, size, import time)
- `asset_import` — trigger reimport

**Step 1: Write and commit**

```bash
git add src/engine/mcp/tools/AssetTools.h src/engine/mcp/tools/AssetTools.cpp
git commit -m "feat(mcp): implement asset management tools (list, info, import)"
```

---

### Task 5.2: Game runtime tools (optional, when Game uses MCP)

**Files:**
- Create: `src/engine/mcp/tools/GameTools.h`
- Create: `src/engine/mcp/tools/GameTools.cpp`

Tools:
- `game_get_state` — running/paused, current frame, game mode info
- `game_simulate` — advance N frames
- `game_get_input_state` — current input device states
- `game_override_variable` — runtime variable override

**Step 1: Write and commit**

```bash
git add src/engine/mcp/tools/GameTools.h src/engine/mcp/tools/GameTools.cpp
git commit -m "feat(mcp): implement game runtime tools (state, simulate, input)"
```

---

### Task 5.3: TransportTCP full implementation

**Files:**
- Modify: `src/engine/mcp/transport/TransportTCP.h`
- Modify: `src/engine/mcp/transport/TransportTCP.cpp`

**Step 1: Implement TCP transport using platform sockets**

Replace the stub with full implementation:
- Server socket (accept one client connection)
- JSON-RPC message framing (newline-delimited)
- SSE for server-to-client push events
- Platform: Winsock2 on Windows, POSIX on Linux

**Step 2: Add CommandLineParser support**

In `src/engine/app/CommandLineParser.h`, register:
- `--mcp` — enable MCP subsystem
- `--mcp-transport=stdio|tcp` — transport selection
- `--mcp-port=<port>` — TCP port (default 3100)

In Engine::Initialize(), read these flags and configure transport accordingly.

**Step 3: Build and commit**

```bash
git add src/engine/mcp/transport/TransportTCP.h src/engine/mcp/transport/TransportTCP.cpp src/engine/app/CommandLineParser.h src/engine/app/CommandLineParser.cpp
git commit -m "feat(mcp): implement TCP transport and command-line flags for MCP"
```

---

## Phase 6: 测试 + 文档 + 收尾

### Task 6.1: Unit tests for MCP core

**Files:**
- Create: `tests/mcp/test_transport_stdio.cpp`
- Create: `tests/mcp/test_mcp_json.cpp`
- Create: `tests/mcp/test_delta_tracker.cpp`
- Create: `tests/mcp/test_tool_registry.cpp`

Test coverage:
- TransportStdio: send/receive JSON messages
- MCPJson: parse valid/invalid JSON-RPC messages
- DeltaTracker: hash computation, delta detection, threshold behavior
- ToolRegistry: register, find, list, categorize tools

### Task 6.2: Integration test

**Files:**
- Create: `tests/mcp/test_mcp_integration.cpp`
- Modify: `CMakeLists.txt` (enable PRISMA_BUILD_TESTING)

Integration test:
1. Create a headless Engine
2. Register MCPSubSystem + mock tools
3. Send JSON-RPC messages via TransportStdio
4. Verify tool execution and responses
5. Test delta tracking with state mutations

### Task 6.3: Documentation

**Files:**
- Create: `docs/PrismaMCP.md`

Documentation covers:
- MCP 架构概览 (组件图 + 数据流)
- 工具清单 (名称、描述、参数、分类)
- 启动方式 (stdio / TCP 模式)
- Agent 工作流 (编译-运行-查询循环)
- Token 优化指南 (字段过滤、增量、预算)
- 扩展指南 (如何注册自定义工具)
- 协议参考 (Prisma MCP 扩展能力)

### Task 6.4: Final verification

1. Build with `PRISMA_ENABLE_MCP=ON`: `cmake --preset engine-windows-x64-debug && cmake --build --preset engine-windows-x64-debug`
2. Build with `PRISMA_ENABLE_MCP=OFF`: verify it compiles without MCP
3. Run editor: `cmake --preset editor-windows-x64-debug && cmake --build --preset editor-windows-x64-debug`
4. Run tests: `ctest --preset test-windows-x64-debug`

---

## 总结

| 项目 | 值 |
|------|-----|
| 新增文件 | ~30 .h/.cpp 文件 |
| 新增依赖 | xxhash (xxh3_64) |
| 新增编译选项 | `PRISMA_ENABLE_MCP` (默认 ON) |
| 核心框架代码 | ~1500 行 (传输层 + 协议 + Session) |
| 工具代码 | ~2000 行 (全部工具实现) |
| 测试代码 | ~800 行 |
| 总计估计 | ~5 天 |
