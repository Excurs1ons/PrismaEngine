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
    bool Send(const glz::json_t& message) override;
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
