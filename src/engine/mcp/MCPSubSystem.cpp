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
    }

    LOG_INFO("MCP", "MCP subsystem ready. Transport: {} | {} tools registered",
        m_Server->IsRunning() ? "connected" : "waiting",
        m_Registry.GetToolCount());

    return 0;
}

void MCPSubSystem::Shutdown() {
    if (m_Server) {
        m_Server->Stop();
        m_Server.reset();
        LOG_INFO("MCP", "MCP subsystem shut down");
    }
}

void MCPSubSystem::Update(Timestep ts) {
    (void)ts;
    m_UpdateTick++;
    if (m_UpdateTick % 10 == 0 && m_Session) {
        m_Session->RefreshHashes();
    }
}

} // namespace MCP
} // namespace Prisma
