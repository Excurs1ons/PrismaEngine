#include "ComponentSerializer.h"

namespace Prisma {
namespace MCP {

nlohmann::json ComponentSerializer::Serialize(void* /*componentData*/,
                                                const std::string& /*typeName*/,
                                                const std::vector<std::string>& /*fields*/,
                                                bool /*omitDefaults*/) {
    // TODO: Implement per-component-type serialization
    // For now returns empty object - framework is in place for implementation
    return nlohmann::json::object();
}

bool ComponentSerializer::Deserialize(void* /*componentData*/,
                                        const std::string& /*typeName*/,
                                        const nlohmann::json& /*data*/) {
    return false;
}

nlohmann::json ComponentSerializer::GetFieldSchema(const std::string& /*typeName*/) {
    return nlohmann::json::object();
}

} // namespace MCP
} // namespace Prisma
