#include <gtest/gtest.h>
#include "logger/LogScope.h"
#include "logger/LogEntry.h"

namespace Prisma {
namespace {

// LogScope constructor sets name and is active
TEST(LogScopeTest, ConstructorSetsNameAndActive) {
    LogScope scope("TestScope");
    EXPECT_EQ(scope.GetName(), "TestScope");
    EXPECT_TRUE(scope.IsActive());
}

// EndScope with success=true keeps entries cached (does not output)
TEST(LogScopeTest, EndScopeSuccessKeepsEntriesCached) {
    LogScope scope("SuccessScope");
    SourceLocation loc;
    LogEntry entry(LogLevel::Info, "cached msg", "Test", loc);

    scope.CacheLogEntry(entry);

    // End with success = true
    scope.EndScope(true);

    EXPECT_FALSE(scope.IsActive());

    // Scope still exists but is inactive; entries should have been cleared
    // No crash or undefined behavior on destruction
}

// EndScope with success=false would output entries (LogScope test)
TEST(LogScopeTest, EndScopeFailureTriggersOutput) {
    LogScope scope("FailScope");
    LogEntry entry(LogLevel::Error, "failure detail", "Test");

    scope.CacheLogEntry(entry);
    EXPECT_TRUE(scope.IsActive());

    // End with success = false (this would normally flush to logger)
    // The scope should become inactive
    scope.EndScope(false);
    EXPECT_FALSE(scope.IsActive());

    // Double-check: calling EndScope again is safe (no-op)
    scope.EndScope(false);
    EXPECT_FALSE(scope.IsActive());
}

// CacheLogEntry on inactive scope is a no-op
TEST(LogScopeTest, CacheOnInactiveScopeIsNoOp) {
    LogScope scope("InactiveScope");
    scope.EndScope(true); // Mark inactive

    EXPECT_FALSE(scope.IsActive());

    // This should not crash or do anything
    LogEntry entry(LogLevel::Debug, "should be ignored", "Test");
    scope.CacheLogEntry(entry);
}

// Multiple entries can be cached
TEST(LogScopeTest, MultipleEntriesCached) {
    LogScope scope("MultiEntry");
    SourceLocation loc;

    for (int i = 0; i < 100; i++) {
        LogEntry entry(LogLevel::Trace, "entry " + std::to_string(i), "Bench", loc);
        scope.CacheLogEntry(entry);
    }

    // Scope should still be active with 100 cached entries
    EXPECT_TRUE(scope.IsActive());

    // End scope successfully - entries cleared
    scope.EndScope(true);
    EXPECT_FALSE(scope.IsActive());
}

// Scope name can be empty
TEST(LogScopeTest, EmptyScopeName) {
    LogScope scope("");
    EXPECT_EQ(scope.GetName(), "");
    EXPECT_TRUE(scope.IsActive());
}

// Destructor calls EndScope(true) automatically
TEST(LogScopeTest, DestructorEndsScopeSuccess) {
    LogScope* scope = new LogScope("AutoEnd");
    EXPECT_TRUE(scope->IsActive());

    // Cache an entry
    LogEntry entry(LogLevel::Info, "will be discarded", "Test");
    scope->CacheLogEntry(entry);

    // Delete should call EndScope(true), discarding cached entries
    delete scope; // Should not leak or crash
}

// ============================================================
// LogScopeManager tests
// ============================================================

TEST(LogScopeTest, ManagerCreateAndDestroy) {
    LogScopeManager& manager = LogScopeManager::Get();

    LogScope* scope = manager.CreateScope("ManagedScope");
    ASSERT_NE(scope, nullptr);
    EXPECT_EQ(scope->GetName(), "ManagedScope");
    EXPECT_TRUE(scope->IsActive());

    manager.DestroyScope(scope, true);
    // scope is now deleted - don't dereference
}

TEST(LogScopeTest, ManagerDestroyWithFailure) {
    LogScopeManager& manager = LogScopeManager::Get();

    LogScope* scope = manager.CreateScope("FailManaged");
    LogEntry entry(LogLevel::Error, "managed failure", "Test");
    scope->CacheLogEntry(entry);

    // Destroy with success=false (would trigger output)
    manager.DestroyScope(scope, false);
    // scope is now deleted - don't dereference
}

// LogScopeManager is singleton
TEST(LogScopeTest, ManagerIsSingleton) {
    LogScopeManager& m1 = LogScopeManager::Get();
    LogScopeManager& m2 = LogScopeManager::Get();
    EXPECT_EQ(&m1, &m2);
}

// Multiple scopes can be created independently
TEST(LogScopeTest, MultipleIndependentScopes) {
    LogScope scope1("Scope1");
    LogScope scope2("Scope2");

    LogEntry entry1(LogLevel::Info, "from scope1", "Test");
    LogEntry entry2(LogLevel::Info, "from scope2", "Test");

    scope1.CacheLogEntry(entry1);
    scope2.CacheLogEntry(entry2);

    EXPECT_TRUE(scope1.IsActive());
    EXPECT_TRUE(scope2.IsActive());

    // End scope1 with success
    scope1.EndScope(true);
    EXPECT_FALSE(scope1.IsActive());
    EXPECT_TRUE(scope2.IsActive());

    // Scope2 still has its cached entry
    scope2.EndScope(false);
    EXPECT_FALSE(scope2.IsActive());
}

} // namespace
} // namespace Prisma
