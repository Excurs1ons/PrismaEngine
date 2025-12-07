#include "pch.h"
#include "LogEntry.h"
#include "LogScope.h"
#include "Logger.h"

#include <mutex>

LogScope::LogScope(const std::string& scopeName)
    : m_scopeName(scopeName) {
}

LogScope::~LogScope() {
    // 如果作用域仍未结束，则默认为成功（不输出日志）
    if (m_active) {
        EndScope(true);
    }
}

void LogScope::EndScope(bool success) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_active) {
        return; // 已经结束过
    }
    
    m_active = false;
    
    // 只有在失败时才输出缓存的日志
    if (!success) {
        for (const auto& entry : m_cachedEntries) {
            Logger::GetInstance().WriteEntry(entry);
        }
    }
    
    m_cachedEntries.clear();
}

void LogScope::CacheLogEntry(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_active) {
        m_cachedEntries.push_back(entry);
    }
}

LogScope* LogScopeManager::CreateScope(const std::string& scopeName) {
    std::lock_guard<std::mutex> lock(m_scopesMutex);
    return new LogScope(scopeName);
}

void LogScopeManager::DestroyScope(LogScope* scope, bool success) {
    if (scope) {
        scope->EndScope(success);
        delete scope;
    }
}