#include "MCPServer.h"
#include "Logger.h"

namespace Prisma {
namespace MCP {

MCPServer::MCPServer(std::unique_ptr<Transport> transport)
    : m_Transport(std::move(transport)) {}

MCPServer::~MCPServer() { Stop(); }

bool MCPServer::Start(ToolRegistry* registry) {
    if (!m_Transport || !registry) return false;
    m_Registry = registry;
    m_Running = true;

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

    // Notification (no "id" field)
    if (!msg.contains("id") || msg["id"].is_null()) {
        std::string method = msg.value("method", "");
        auto it = m_NotificationHandlers.find(method);
        if (it != m_NotificationHandlers.end()) {
            it->second(msg.value("params", nlohmann::json::object()));
        }
        return;
    }

    // Request
    auto req = MCPRequest::fromJson(msg);
    if (!req) {
        sendResponse(MCPResponse::Error(nullptr, ErrorCode::InvalidRequest, "Invalid request"));
        return;
    }
    handleRequest(*req);
}

void MCPServer::handleRequest(const MCPRequest& req) {
    if (req.method == "initialize") {
        handleInitialize(req);
    } else if (req.method == "notifications/initialized") {
        sendResponse(MCPResponse::Success(req.id, {{"ok", true}}));
    } else if (req.method == "tools/list" || req.method == "mcp/discover_all_tools") {
        handleListTools(req);
    } else if (req.method == "mcp/discover_tools") {
        handleListTools(req);
    } else if (req.method == "tools/call") {
        handleCallTool(req);
    } else if (req.method == "mcp/get_state_hash") {
        handleGetStateHash(req);
    } else {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::MethodNotFound,
            "Unknown method: " + req.method));
    }
}

void MCPServer::handleInitialize(const MCPRequest& req) {
    auto clientInfo = req.params.value("clientInfo", nlohmann::json::object());

    LOG_INFO("MCP", "Client connected: {} v{}",
        clientInfo.value("name", "unknown"),
        clientInfo.value("version", "?"));

    int maxTokens = req.params.value("maxTokensPerResponse", 2000);
    if (m_Session) {
        m_Session->SetTokenBudget(maxTokens);
    }

    nlohmann::json result = {
        {"protocolVersion", "2025-03-26"},
        {"capabilities", {
            {"tools", {{"listChanged", false}}},
            {"prisma_extensions", {
                "delta",
                "field_filter",
                "paginate",
                "value_omit",
                "session_cache"
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
    for (const auto* tool : tools) {
        nlohmann::json t;
        t["name"]        = tool->GetName();
        t["description"] = tool->GetDescription();
        t["inputSchema"] = tool->GetInputSchema();
        t["category"]    = tool->GetCategory();
        toolsJson.push_back(std::move(t));
    }

    sendResponse(MCPResponse::Success(req.id, {{"tools", toolsJson}}));
}

void MCPServer::handleCallTool(const MCPRequest& req) {
    if (!m_Registry) {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::InternalError, "Tool registry not available"));
        return;
    }

    std::string toolName = req.params.value("name", "");
    auto arguments = req.params.value("arguments", nlohmann::json::object());

    // Check for hash-based delta first
    std::string knownHash = arguments.value("_known_hash", std::string());
    if (!knownHash.empty() && m_Session) {
        auto delta = m_Session->TryGetDelta(knownHash, toolName, arguments);
        if (delta.is_object() && delta.value("_delta", false)) {
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
    sendResponse(MCPResponse::Success(req.id, {
        {"root_hash", m_Session->GetRootHashString()},
        {"version",   m_Session->GetVersion()}
    }));
}

void MCPServer::sendResponse(const MCPResponse& resp) {
    if (m_Transport && m_Transport->IsConnected()) {
        m_Transport->Send(resp.toJson());
    }
}

} // namespace MCP
} // namespace Prisma
