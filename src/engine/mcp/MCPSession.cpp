#include "MCPSession.h"
#include <sstream>

namespace Prisma {
namespace MCP {

MCPSession::MCPSession() = default;

nlohmann::json MCPSession::TryGetDelta(const std::string& knownHashStr,
                                        const std::string& toolName,
                                        const nlohmann::json& args) {
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
        return {{"_unchanged", true}};
    }

    if (!delta.snapshot.is_null()) {
        return {{"_full_snapshot", true}, {"root_hash", GetRootHashString()}};
    }

    nlohmann::json result = delta.delta;
    result["root_hash"] = GetRootHashString();
    result["_delta"] = true;
    return result;
}

void MCPSession::RefreshHashes() {
    m_DeltaTracker.GetRootHash();
}

} // namespace MCP
} // namespace Prisma
