#pragma once
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <optional>

namespace Prisma {
namespace MCP {

struct MCPRequest {
    std::string jsonrpc = "2.0";
    std::string method;
    glz::json_t params = glz::json_t::object_t{};
    glz::json_t id = {};

    glz::json_t toJson() const;
    static std::optional<MCPRequest> fromJson(const glz::json_t& j);
};

struct MCPResponse {
    std::string jsonrpc = "2.0";
    glz::json_t id = {};
    glz::json_t result = glz::json_t::object_t{};
    std::optional<glz::json_t> error = std::nullopt;

    glz::json_t toJson() const;
    static MCPResponse Success(glz::json_t id, glz::json_t result);
    static MCPResponse Error(glz::json_t id, int code,
                             const std::string& message,
                             glz::json_t data = {});
};

namespace ErrorCode {
    constexpr int ParseError     = -32700;
    constexpr int InvalidRequest = -32600;
    constexpr int MethodNotFound = -32601;
    constexpr int InvalidParams  = -32602;
    constexpr int InternalError  = -32603;
}

} // namespace MCP
} // namespace Prisma
