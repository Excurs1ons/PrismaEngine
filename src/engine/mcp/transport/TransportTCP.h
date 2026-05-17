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
    bool Send(const glz::json_t& message) override;
    bool IsConnected() const override;
    std::string_view GetName() const override { return "tcp"; }

private:
    void acceptLoop();

    uint16_t m_Port;
    std::thread m_AcceptThread;
    std::thread m_ReadThread;
    std::atomic<bool> m_Running{false};
    MCPMessageHandler m_Handler;
    int m_ServerSocket = -1;
    int m_ClientSocket = -1;
};

} // namespace MCP
} // namespace Prisma
