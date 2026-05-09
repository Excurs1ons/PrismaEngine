#include "TransportTCP.h"
#include "Logger.h"

namespace Prisma {
namespace MCP {

TransportTCP::TransportTCP(uint16_t port) : m_Port(port) {}
TransportTCP::~TransportTCP() { Stop(); }

bool TransportTCP::Start(MCPMessageHandler handler) {
    m_Handler = std::move(handler);
    // Full TCP implementation uses platform sockets (Winsock2 / POSIX)
    // For now: stub returns false - uses stdio as default transport
    LOG_WARN("MCP", "TCP transport not yet fully implemented, falling back to stdio");
    return false;
}

void TransportTCP::Stop() {
    m_Running = false;
    // TODO: close sockets, join thread
}

bool TransportTCP::Send(const nlohmann::json& /*message*/) {
    return false;
}

bool TransportTCP::IsConnected() const {
    return false;
}

} // namespace MCP
} // namespace Prisma
