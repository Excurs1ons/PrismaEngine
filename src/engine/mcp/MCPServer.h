#pragma once
#include "transport/Transport.h"
#include "serialization/MCPJson.h"
#include "MCPTool.h"
#include "MCPSession.h"
#include <memory>
#include <unordered_map>
#include <functional>
#include <atomic>

namespace Prisma {
namespace MCP {

class ENGINE_API MCPServer {
public:
    explicit MCPServer(std::unique_ptr<Transport> transport);
    ~MCPServer();

    MCPServer(const MCPServer&) = delete;
    MCPServer& operator=(const MCPServer&) = delete;

    bool Start(ToolRegistry* registry);
    void Stop();
    bool IsRunning() const { return m_Running; }

    void SetSession(std::shared_ptr<MCPSession> session) { m_Session = std::move(session); }
    MCPSession* GetSession() { return m_Session.get(); }

private:
    void onMessage(const glz::json_t& msg);
    void handleRequest(const MCPRequest& req);
    void handleInitialize(const MCPRequest& req);
    void handleListTools(const MCPRequest& req);
    void handleCallTool(const MCPRequest& req);
    void handleGetStateHash(const MCPRequest& req);
    void sendResponse(const MCPResponse& resp);

    std::unique_ptr<Transport> m_Transport;
    ToolRegistry* m_Registry = nullptr;
    std::shared_ptr<MCPSession> m_Session;
    std::atomic<bool> m_Running{false};

    using NotificationHandler = std::function<void(const glz::json_t&)>;
    std::unordered_map<std::string, NotificationHandler> m_NotificationHandlers;
};

} // namespace MCP
} // namespace Prisma
