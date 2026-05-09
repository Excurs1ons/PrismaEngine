#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace Prisma {
namespace MCP {

class ComponentSerializer {
public:
    // Serialize component data with optional field filtering and default-value omission
    static nlohmann::json Serialize(void* componentData, const std::string& typeName,
                                    const std::vector<std::string>& fields = {},
                                    bool omitDefaults = true);

    // Deserialize from JSON
    static bool Deserialize(void* componentData, const std::string& typeName,
                            const nlohmann::json& data);

    // Get field schema for a component type
    static nlohmann::json GetFieldSchema(const std::string& typeName);
};

} // namespace MCP
} // namespace Prisma
