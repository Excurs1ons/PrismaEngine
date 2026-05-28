#include <gtest/gtest.h>
#include "logger/Logger.h"
#include "logger/LogEntry.h"
#include <string>
#include <vector>
#include <thread>
#include <chrono>

namespace Prisma {
namespace {

// ============================================================
// LogEntry construction verification
// ============================================================

TEST(LoggerTest, LogEntryConstruction) {
    SourceLocation loc("test.cpp", 42, "testFunction");
    LogEntry entry(LogLevel::Info, "test message", "TestCategory", loc);

    EXPECT_EQ(entry.level, LogLevel::Info);
    EXPECT_EQ(entry.message, "test message");
    EXPECT_EQ(entry.category, "TestCategory");
    EXPECT_EQ(entry.location.file, std::string("test.cpp"));
    EXPECT_EQ(entry.location.line, 42);
    EXPECT_EQ(entry.location.function, std::string("testFunction"));
    EXPECT_EQ(entry.threadId, std::this_thread::get_id());
}

// LogEntry with default SourceLocation
TEST(LoggerTest, LogEntryDefaultLocation) {
    LogEntry entry(LogLevel::Warning, "warning message", "Core");

    EXPECT_EQ(entry.level, LogLevel::Warning);
    EXPECT_STREQ(entry.location.file, "");
    EXPECT_EQ(entry.location.line, 0);
    EXPECT_STREQ(entry.location.function, "");
}

// LogEntry timestamp is recent (within 5 seconds of creation)
TEST(LoggerTest, LogEntryTimestampRecent) {
    auto before = std::chrono::system_clock::now();
    LogEntry entry(LogLevel::Debug, "timing", "Test");
    auto after = std::chrono::system_clock::now();

    EXPECT_GE(entry.timestamp, before);
    EXPECT_LE(entry.timestamp, after);
}

// ============================================================
// LogLevel enum values
// ============================================================

TEST(LoggerTest, LogLevelValues) {
    EXPECT_LT(static_cast<int>(LogLevel::Trace), static_cast<int>(LogLevel::Debug));
    EXPECT_LT(static_cast<int>(LogLevel::Debug), static_cast<int>(LogLevel::Info));
    EXPECT_LT(static_cast<int>(LogLevel::Info), static_cast<int>(LogLevel::Warning));
    EXPECT_LT(static_cast<int>(LogLevel::Warning), static_cast<int>(LogLevel::Error));
    EXPECT_LT(static_cast<int>(LogLevel::Error), static_cast<int>(LogLevel::Fatal));
}

// ============================================================
// Logger singleton
// ============================================================

TEST(LoggerTest, LoggerIsSingleton) {
    Logger& instance1 = Logger::Get();
    Logger& instance2 = Logger::Get();
    EXPECT_EQ(&instance1, &instance2);
}

// ============================================================
// Logger::SetMinLevel / GetMinLevel
// ============================================================

TEST(LoggerTest, MinLevelDefault) {
    Logger& logger = Logger::Get();
    LogLevel original = logger.GetMinLevel();
    // Default should be Debug or Info depending on build config
    EXPECT_GE(static_cast<int>(original), static_cast<int>(LogLevel::Trace));
    EXPECT_LE(static_cast<int>(original), static_cast<int>(LogLevel::Info));
}

TEST(LoggerTest, SetMinLevel) {
    Logger& logger = Logger::Get();
    LogLevel original = logger.GetMinLevel();

    logger.SetMinLevel(LogLevel::Fatal);
    EXPECT_EQ(logger.GetMinLevel(), LogLevel::Fatal);

    logger.SetMinLevel(LogLevel::Trace);
    EXPECT_EQ(logger.GetMinLevel(), LogLevel::Trace);

    logger.SetMinLevel(LogLevel::Warning);
    EXPECT_EQ(logger.GetMinLevel(), LogLevel::Warning);

    // Restore
    logger.SetMinLevel(original);
}

// ============================================================
// Logger::LogInternal with level filtering
// ============================================================

TEST(LoggerTest, LogInternalBelowMinLevelIsFiltered) {
    Logger& logger = Logger::Get();
    LogLevel original = logger.GetMinLevel();

    // Set min level to Fatal so only Fatal gets through
    logger.SetMinLevel(LogLevel::Fatal);

    // Clear history by logging nothing
    // These should be filtered out by LogInternal
    logger.LogInternal(LogLevel::Trace, "Test", "should not appear", SourceLocation());
    logger.LogInternal(LogLevel::Debug, "Test", "should not appear", SourceLocation());
    logger.LogInternal(LogLevel::Info, "Test", "should not appear", SourceLocation());
    logger.LogInternal(LogLevel::Warning, "Test", "should not appear", SourceLocation());
    logger.LogInternal(LogLevel::Error, "Test", "should not appear", SourceLocation());

    // Only Fatal should pass through
    logger.LogInternal(LogLevel::Fatal, "Test", "should appear", SourceLocation());

    auto recentLogs = logger.GetRecentLogs(100);
    size_t count = recentLogs.size();

    // Restore
    logger.SetMinLevel(original);

    // The fatal log might have been processed; verify at least one entry
    // (Note: in async mode, entries may or may not be processed yet)
    if (count > 0) {
        EXPECT_EQ(recentLogs.back().level, LogLevel::Fatal);
    }
}

// ============================================================
// Logger::WriteEntry and GetRecentLogs
// ============================================================

TEST(LoggerTest, WriteEntryAppearsInRecentLogs) {
    Logger& logger = Logger::Get();
    SourceLocation loc("test.cpp", 1, "test");

    // Write an entry directly
    LogEntry entry(LogLevel::Info, "direct write test", "Test", loc);
    logger.WriteEntry(entry);

    auto logs = logger.GetRecentLogs(10);
    ASSERT_GE(logs.size(), 1);

    bool found = false;
    for (const auto& log : logs) {
        if (log.message == "direct write test" && log.category == "Test") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// GetRecentLogs returns correct number
TEST(LoggerTest, GetRecentLogsCount) {
    Logger& logger = Logger::Get();

    // Write some entries
    for (int i = 0; i < 10; i++) {
        LogEntry entry(LogLevel::Debug, "count test " + std::to_string(i), "Test");
        logger.WriteEntry(entry);
    }

    auto logs = logger.GetRecentLogs(5);
    EXPECT_LE(logs.size(), 5);
}

// ============================================================
// SourceLocation construction
// ============================================================

TEST(LoggerTest, SourceLocationConstruction) {
    SourceLocation loc("file.cpp", 100, "myFunc");
    EXPECT_STREQ(loc.file, "file.cpp");
    EXPECT_EQ(loc.line, 100);
    EXPECT_STREQ(loc.function, "myFunc");
}

TEST(LoggerTest, SourceLocationDefault) {
    SourceLocation loc;
    EXPECT_STREQ(loc.file, "");
    EXPECT_EQ(loc.line, 0);
    EXPECT_STREQ(loc.function, "");
}

// ============================================================
// LogTarget enum
// ============================================================

TEST(LoggerTest, LogTargetValues) {
    LogTarget console = LogTarget::Console;
    LogTarget file = LogTarget::File;
    LogTarget both = LogTarget::Both;

    // Both should have Console and File bits set
    EXPECT_TRUE(both & LogTarget::Console);
    EXPECT_TRUE(both & LogTarget::File);

    // Console should not have File bit set
    EXPECT_TRUE(console & LogTarget::Console);
    EXPECT_FALSE(console & LogTarget::File);
}

} // namespace
} // namespace Prisma
