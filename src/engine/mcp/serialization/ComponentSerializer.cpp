#include "ComponentSerializer.h"

namespace Prisma {
namespace MCP {

glz::json_t ComponentSerializer::Serialize(void* /*componentData*/,
                                                const std::string& /*typeName*/,
                                                const std::vector<std::string>& /*fields*/,
                                                bool /*omitDefaults*/) {
    // TODO: Implement per-component-type serialization
    // For now returns empty object - framework is in place for implementation
    return glz::json_t::object_t{};
}

bool ComponentSerializer::Deserialize(void* /*componentData*/,
                                        const std::string& /*typeName*/,
                                        const glz::json_t& /*data*/) {
    return false;
}

glz::json_t ComponentSerializer::GetFieldSchema(const std::string& /*typeName*/) {
    return glz::json_t::object_t{};
}

} // namespace MCP
} // namespace Prisma
