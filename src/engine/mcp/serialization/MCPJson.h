#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <optional>

namespace Prisma {
namespace MCP {

struct MCPRequest {
    std::string jsonrpc = "2.0";
    std::string method;
    nlohmann::json params = nlohmann::json::object();
    nlohmann::json id = nullptr;

    nlohmann::json toJson() const;
    static std::optional<MCPRequest> fromJson(const nlohmann::json& j);
};

struct MCPResponse {
    std::string jsonrpc = "2.0";
    nlohmann::json id = nullptr;
    nlohmann::json result = nlohmann::json::object();
    std::optional<nlohmann::json> error = std::nullopt;

    nlohmann::json toJson() const;
    static MCPResponse Success(nlohmann::json id, nlohmann::json result);
    static MCPResponse Error(nlohmann::json id, int code,
                             const std::string& message,
                             nlohmann::json data = nullptr);
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
