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

    m_NotificationHandlers["notifications/initialized"] = [](const glz::json_t&) {};

    return m_Transport->Start([this](const glz::json_t& msg) {
        onMessage(msg);
    });
}

void MCPServer::Stop() {
    m_Running = false;
    if (m_Transport) m_Transport->Stop();
}

void MCPServer::onMessage(const glz::json_t& msg) {
    if (!msg.is_object()) return;

    auto& obj = msg.get_object();
    // Notification (no "id" field)
    if (!obj.contains("id") || obj.at("id").is_null()) {
        std::string method = obj.contains("method") ? obj.at("method").get_string() : "";
        auto it = m_NotificationHandlers.find(method);
        if (it != m_NotificationHandlers.end()) {
            it->second(obj.contains("params") ? obj.at("params") : glz::json_t::object_t{});
        }
        return;
    }

    // Request
    auto req = MCPRequest::fromJson(msg);
    if (!req) {
        sendResponse(MCPResponse::Error({}, ErrorCode::InvalidRequest, "Invalid request"));
        return;
    }
    handleRequest(*req);
}

void MCPServer::handleRequest(const MCPRequest& req) {
    if (req.method == "initialize") {
        handleInitialize(req);
    } else if (req.method == "notifications/initialized") {
        sendResponse(MCPResponse::Success(req.id, glz::json_t::object_t{{"ok", true}}));
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
    auto& paramsObj = req.params.get_object();
    auto clientInfo = paramsObj.contains("clientInfo") ? paramsObj.at("clientInfo").get_object() : glz::json_t::object_t{};

    LOG_INFO("MCP", "Client connected: {0} v{1}",
        clientInfo.contains("name") ? clientInfo.at("name").get_string() : "unknown",
        clientInfo.contains("version") ? clientInfo.at("version").get_string() : "?");

    int maxTokens = paramsObj.contains("maxTokensPerResponse") ? static_cast<int>(paramsObj.at("maxTokensPerResponse").get_number()) : 2000;
    if (m_Session) {
        m_Session->SetTokenBudget(maxTokens);
    }

    glz::json_t result = glz::json_t::object_t{
        {"protocolVersion", "2025-03-26"},
        {"capabilities", glz::json_t::object_t{
            {"tools", glz::json_t::object_t{{"listChanged", false}}},
            {"prisma_extensions", glz::json_t::array_t{
                "delta",
                "field_filter",
                "paginate",
                "value_omit",
                "session_cache"
            }}
        }},
        {"serverInfo", glz::json_t::object_t{
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

    auto& paramsObj = req.params.get_object();
    auto filter = paramsObj.contains("category") ? paramsObj.at("category").get_string() : std::string();
    auto tools = m_Registry->GetToolList(filter);

    glz::json_t::array_t toolsJson;
    for (const auto* tool : tools) {
        glz::json_t::object_t t;
        t["name"]        = std::string(tool->GetName());
        t["description"] = std::string(tool->GetDescription());
        t["inputSchema"] = tool->GetInputSchema();
        t["category"]    = std::string(tool->GetCategory());
        toolsJson.push_back(std::move(t));
    }

    sendResponse(MCPResponse::Success(req.id, glz::json_t::object_t{{"tools", std::move(toolsJson)}}));
}

void MCPServer::handleCallTool(const MCPRequest& req) {
    if (!m_Registry) {
        sendResponse(MCPResponse::Error(req.id, ErrorCode::InternalError, "Tool registry not available"));
        return;
    }

    auto& paramsObj = req.params.get_object();
    std::string toolName = paramsObj.contains("name") ? paramsObj.at("name").get_string() : "";
    auto arguments = paramsObj.contains("arguments") ? paramsObj.at("arguments") : glz::json_t::object_t{};

    // Check for hash-based delta first
    auto& argsObj = arguments.get_object();
    std::string knownHash = argsObj.contains("_known_hash") ? argsObj.at("_known_hash").get_string() : std::string();
    if (!knownHash.empty() && m_Session) {
        auto delta = m_Session->TryGetDelta(knownHash, toolName, arguments);
        if (delta.is_object() && delta.get_object().contains("_delta") && delta.get_object().at("_delta").get_boolean()) {
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
    sendResponse(MCPResponse::Success(req.id, glz::json_t::object_t{
        {"root_hash", m_Session->GetRootHashString()},
        {"version",   static_cast<double>(m_Session->GetVersion())}
    }));
}

void MCPServer::sendResponse(const MCPResponse& resp) {
    if (m_Transport && m_Transport->IsConnected()) {
        m_Transport->Send(resp.toJson());
    }
}

} // namespace MCP
} // namespace Prisma
