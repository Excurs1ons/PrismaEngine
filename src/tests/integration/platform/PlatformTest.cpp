#include <gtest/gtest.h>
#include "platform/Platform.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <filesystem>

namespace Prisma {
namespace {

// ==================== 时间函数测试 ====================

// 测试 GetTimeMicroseconds 返回非零且递增的时间值
TEST(PlatformTimeTest, GetTimeMicroseconds) {
    uint64_t t1 = Platform::GetTimeMicroseconds();
    ASSERT_GT(t1, 0u);

    // 睡眠 1ms 确保时间流逝
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t t2 = Platform::GetTimeMicroseconds();

    EXPECT_GT(t2, t1);
    // 至少应增加 1000 微秒（1ms）
    EXPECT_GE(t2 - t1, 500u);  // 允许 0.5ms 误差
}

// 测试 GetTimeSeconds 返回非零且递增的时间值（精度为秒级）
TEST(PlatformTimeTest, GetTimeSeconds) {
    double t1 = Platform::GetTimeSeconds();
    ASSERT_GT(t1, 0.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double t2 = Platform::GetTimeSeconds();

    EXPECT_GT(t2, t1);
    EXPECT_GE(t2 - t1, 0.001);  // 至少差 0.001 秒
}

// 测试 SleepMilliseconds 的实际睡眠时间在合理范围内
TEST(PlatformTimeTest, SleepMilliseconds) {
    constexpr uint32_t kSleepMs = 20;

    auto start = std::chrono::steady_clock::now();
    Platform::SleepMilliseconds(kSleepMs);
    auto elapsed = std::chrono::steady_clock::now() - start;

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    // 实际睡眠时间应至少接近请求值（允许调度误差）
    EXPECT_GE(elapsedMs, kSleepMs / 2u);
    // 不应超出太多（10 倍容差，防止极端调度延迟误判）
    EXPECT_LE(elapsedMs, kSleepMs * 10u);
}

// 测试连续多次 Sleep 的累计时间
TEST(PlatformTimeTest, SleepMultipleTimes) {
    auto start = std::chrono::steady_clock::now();

    Platform::SleepMilliseconds(5);
    Platform::SleepMilliseconds(5);
    Platform::SleepMilliseconds(5);

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    EXPECT_GE(elapsedMs, 5u);   // 至少应有一次睡眠的时长
}

// ==================== 系统信息测试 ====================

// 测试 GetProcessId 返回有效的进程 ID
TEST(PlatformSysInfoTest, GetProcessId) {
    uint32_t pid = Platform::GetProcessId();
    EXPECT_GT(pid, 0u);
    // PID 通常不会超过 2^22（约 400 万），但不同系统有差异
    // 只需要验证它是一个合理的正数
    EXPECT_LT(pid, 10000000u);
}

// 测试硬件并发数（CPU 核心数）的合理性
TEST(PlatformSysInfoTest, HardwareConcurrency) {
    unsigned int coreCount = std::thread::hardware_concurrency();
    EXPECT_GT(coreCount, 0u);
    EXPECT_LE(coreCount, 256u);  // 合理上限
}

// ==================== 路径函数测试 ====================

// 测试 GetExecutablePath 返回非空路径
TEST(PlatformPathTest, GetExecutablePath) {
    const char* path = Platform::GetExecutablePath();
    ASSERT_NE(path, nullptr);
    EXPECT_GT(strlen(path), 0u);
    // 路径应包含分隔符
    EXPECT_TRUE(strchr(path, '/') != nullptr);
}

// 测试 SetCurrentDirectory 能正确切换工作目录
TEST(PlatformPathTest, SetCurrentDirectory) {
    std::error_code ec;
    std::filesystem::path originalPath = std::filesystem::current_path(ec);
    ASSERT_FALSE(ec) << "无法获取当前工作目录";

    // 切换到 /tmp 目录
    EXPECT_TRUE(Platform::SetCurrentDirectory("/tmp"));

    std::filesystem::path newPath = std::filesystem::current_path(ec);
    EXPECT_EQ(newPath, std::filesystem::path("/tmp"));

    // 恢复原目录
    std::filesystem::current_path(originalPath, ec);
    ASSERT_FALSE(ec) << "无法恢复原工作目录";
}

// 测试 GetTemporaryPath 返回非空路径
TEST(PlatformPathTest, GetTemporaryPath) {
    const char* tmpPath = Platform::GetTemporaryPath();
    ASSERT_NE(tmpPath, nullptr);
    EXPECT_GT(strlen(tmpPath), 0u);
}

// ==================== 文件系统测试 ====================

// 测试 FileExists 对存在的路径返回 true
TEST(PlatformFileTest, FileExistsPositive) {
    EXPECT_TRUE(Platform::FileExists("/tmp"));
    EXPECT_TRUE(Platform::FileExists("/"));
}

// 测试 FileExists 对不存在的路径返回 false
TEST(PlatformFileTest, FileExistsNegative) {
    EXPECT_FALSE(Platform::FileExists("/nonexistent_path_xyz_123"));
}

} // namespace
} // namespace Prisma
