#pragma once

namespace Prisma {
namespace MCP {

class TokenBudget {
public:
    void SetMaxTokens(int max) { m_MaxTokens = max; }
    int GetMaxTokens() const { return m_MaxTokens; }

    bool WouldExceed(int estimatedTokens) const {
        return estimatedTokens > m_MaxTokens;
    }

private:
    int m_MaxTokens = 2000;
};

} // namespace MCP
} // namespace Prisma
