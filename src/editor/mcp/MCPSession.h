#pragma once
#include "Export.h"
#include "session/DeltaTracker.h"
#include "session/TokenBudget.h"
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <memory>
#include <atomic>

namespace Prisma {
namespace MCP {

class EDITOR_API MCPSession {
public:
    MCPSession();

    // Hash tracking
    DeltaTracker& GetDeltaTracker() { return m_DeltaTracker; }
    Hash64 GetRootHash() const { return m_DeltaTracker.GetRootHash(); }
    std::string GetRootHashString() const { return m_DeltaTracker.GetRootHashString(); }
    uint64_t GetVersion() const { return m_VersionCounter; }

    // Token budget
    void SetTokenBudget(int maxTokens) { m_TokenBudget.SetMaxTokens(maxTokens); }
    int GetTokenBudget() const { return m_TokenBudget.GetMaxTokens(); }

    // Delta computation
    glz::json_t TryGetDelta(const std::string& knownHashStr, const std::string& toolName,
                               const glz::json_t& args);

    // Period refresh
    void RefreshHashes();
    void MarkDirty() { m_VersionCounter++; }

private:
    DeltaTracker m_DeltaTracker;
    TokenBudget m_TokenBudget;
    std::atomic<uint64_t> m_VersionCounter{0};
};

} // namespace MCP
} // namespace Prisma
