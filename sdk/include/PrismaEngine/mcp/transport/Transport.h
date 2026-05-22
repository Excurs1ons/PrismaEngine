#pragma once
#include <string>
#include <functional>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include "Export.h"

namespace Prisma {
namespace MCP {

using MCPMessageHandler = std::function<void(const glz::json_t& message)>;

class ENGINE_API Transport {
public:
    virtual ~Transport() = default;

    virtual bool Start(MCPMessageHandler handler) = 0;
    virtual void Stop() = 0;
    virtual bool Send(const glz::json_t& message) = 0;
    virtual bool IsConnected() const = 0;
    virtual std::string_view GetName() const = 0;
};

} // namespace MCP
} // namespace Prisma
