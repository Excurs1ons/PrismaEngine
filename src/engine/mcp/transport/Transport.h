#pragma once
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "Export.h"

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
