#include "MCPSession.h"
#include <sstream>

namespace Prisma {
namespace MCP {

MCPSession::MCPSession() = default;

glz::json_t MCPSession::TryGetDelta(const std::string& knownHashStr,
                                        const std::string& toolName,
                                        const glz::json_t& args) {
    Hash64 knownHash = 0;
    std::string h = knownHashStr;
    if (h.size() > 2 && h.substr(0, 2) == "0x") {
        h = h.substr(2);
    }
    if (!h.empty()) {
        knownHash = std::stoull(h, nullptr, 16);
    }

    auto delta = m_DeltaTracker.ComputeDelta(knownHash, toolName, args);
    if (!delta.hasChanges) {
        return glz::json_t::object_t{{"_unchanged", true}};
    }

    if (!delta.snapshot.is_null()) {
        return glz::json_t::object_t{{"_full_snapshot", true}, {"root_hash", GetRootHashString()}};
    }

    glz::json_t result = delta.delta;
    auto& obj = result.get_object();
    obj["root_hash"] = GetRootHashString();
    obj["_delta"] = true;
    return result;
}

void MCPSession::RefreshHashes() {
    m_DeltaTracker.GetRootHash();
}

} // namespace MCP
} // namespace Prisma
