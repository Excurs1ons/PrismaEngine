#pragma once
#include <glaze/glaze.hpp>
#include <glaze/json/json_t.hpp>
#include <string>
#include <vector>

namespace Prisma {
namespace MCP {

class ComponentSerializer {
public:
    // Serialize component data with optional field filtering and default-value omission
    static glz::json_t Serialize(void* componentData, const std::string& typeName,
                                    const std::vector<std::string>& fields = {},
                                    bool omitDefaults = true);

    // Deserialize from JSON
    static bool Deserialize(void* componentData, const std::string& typeName,
                            const glz::json_t& data);

    // Get field schema for a component type
    static glz::json_t GetFieldSchema(const std::string& typeName);
};

} // namespace MCP
} // namespace Prisma
