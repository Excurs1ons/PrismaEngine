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
    const char* GetName() const override { return "MCPSubSystem"; }

    // Tool registration
    template<typename T, typename... Args>
    T* RegisterTool(Args&&... args) {
        auto tool = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = tool.get();
        m_Registry.AddTool(std::move(tool));
        return ptr;
    }

    void RegisterTools(std::vector<std::unique_ptr<MCPTool>> tools) {
        m_Registry.AddTools(std::move(tools));
    }

    ToolRegistry& GetRegistry() { return m_Registry; }
    MCPServer* GetServer() { return m_Server.get(); }
    MCPSession* GetSession() { return m_Session.get(); }

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
