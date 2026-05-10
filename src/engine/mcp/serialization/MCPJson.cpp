#include "MCPJson.h"

namespace Prisma {
namespace MCP {

nlohmann::json MCPRequest::toJson() const {
    return {
        {"jsonrpc", jsonrpc},
        {"method", method},
        {"params", params},
        {"id", id}
    };
}

std::optional<MCPRequest> MCPRequest::fromJson(const nlohmann::json& j) {
    if (!j.is_object() || !j.contains("method") || !j["method"].is_string())
        return std::nullopt;

    MCPRequest req;
    req.jsonrpc = j.value("jsonrpc", "2.0");
    req.method  = j["method"].get<std::string>();
    req.params  = j.value("params", nlohmann::json::object());
    req.id      = j.value("id", nlohmann::json());  // nullptr deduces nullptr_t - use json() instead
    return req;
}

nlohmann::json MCPResponse::toJson() const {
    nlohmann::json j = {{"jsonrpc", jsonrpc}, {"id", id}};
    if (error) {
        j["error"] = *error;
    } else {
        j["result"] = result;
    }
    return j;
}

MCPResponse MCPResponse::Success(nlohmann::json id, nlohmann::json result) {
    MCPResponse r;
    r.id     = std::move(id);
    r.result = std::move(result);
    return r;
}

MCPResponse MCPResponse::Error(nlohmann::json id, int code,
                               const std::string& message,
                               nlohmann::json data) {
    MCPResponse r;
    r.id    = std::move(id);
    r.error = {{"code", code}, {"message", message}};
    if (!data.is_null()) (*r.error)["data"] = std::move(data);
    return r;
}

} // namespace MCP
} // namespace Prisma
