#pragma once

#include <vector>
#include <mutex>
#include <string>

#include "LogEntry.h"

// 日志作用域类，用于缓存特定范围内的日志
// 只有在作用域结束时指定失败的情况下才会输出日志
class LogScope {
public:
    explicit LogScope(const std::string& scopeName);
    ~LogScope();

    // 结束作用域，如果success为false则输出所有缓存的日志
    void EndScope(bool success = true);

    // 添加日志条目到缓存
    void CacheLogEntry(const LogEntry& entry);

    // 获取作用域名称
    const std::string& GetName() const { return m_scopeName; }

    // 检查作用域是否活跃（未结束）
    bool IsActive() const { return m_active; }

private:
    std::string m_scopeName;
    std::vector<LogEntry> m_cachedEntries;
    mutable std::mutex m_mutex;
    bool m_active = true;
};

// 日志作用域管理器，用于管理多个日志作用域
class LogScopeManager {
public:
    static LogScopeManager& GetInstance() {
        static LogScopeManager instance;
        return instance;
    }

    // 创建新的日志作用域
    LogScope* CreateScope(const std::string& scopeName);

    // 结束并销毁指定的作用域
    void DestroyScope(LogScope* scope, bool success = true);

private:
    std::mutex m_scopesMutex;
};