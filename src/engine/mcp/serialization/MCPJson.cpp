#include "MCPJson.h"

namespace Prisma {
namespace MCP {

glz::json_t MCPRequest::toJson() const {
    return glz::json_t::object_t{
        {"jsonrpc", jsonrpc},
        {"method", method},
        {"params", params},
        {"id", id}
    };
}

std::optional<MCPRequest> MCPRequest::fromJson(const glz::json_t& j) {
    if (!j.is_object() || !j.get_object().contains("method") || !j.get_object().at("method").is_string())
        return std::nullopt;

    MCPRequest req;
    auto& obj = j.get_object();
    if (obj.contains("jsonrpc")) req.jsonrpc = obj.at("jsonrpc").get_string();
    req.method  = obj.at("method").get_string();
    if (obj.contains("params")) req.params = obj.at("params");
    if (obj.contains("id")) req.id = obj.at("id");
    return req;
}

glz::json_t MCPResponse::toJson() const {
    glz::json_t::object_t obj{{"jsonrpc", jsonrpc}, {"id", id}};
    if (error) {
        obj["error"] = *error;
    } else {
        obj["result"] = result;
    }
    return obj;
}

MCPResponse MCPResponse::Success(glz::json_t id, glz::json_t result) {
    MCPResponse r;
    r.id     = std::move(id);
    r.result = std::move(result);
    return r;
}

MCPResponse MCPResponse::Error(glz::json_t id, int code,
                               const std::string& message,
                               glz::json_t data) {
    MCPResponse r;
    r.id    = std::move(id);
    r.error = glz::json_t::object_t{{"code", static_cast<double>(code)}, {"message", message}};
    if (!data.is_null()) (*r.error).get_object()["data"] = std::move(data);
    return r;
}

} // namespace MCP
} // namespace Prisma
