#include <gtest/gtest.h>

#include "scripting/CoreCLRHost.h"

namespace Prisma::Scripting {
namespace {

// ============================================================================
// CoreCLRHost — Default State
// ============================================================================
TEST(CoreCLRHostTest, DefaultState) {
    CoreCLRHost host;
    EXPECT_FALSE(host.IsInitialized());
    EXPECT_TRUE(host.GetScriptsDir().empty());
}

// ============================================================================
// CoreCLRHost — Shutdown Without Initialize
// ============================================================================
TEST(CoreCLRHostTest, ShutdownWithoutInitNoCrash) {
    CoreCLRHost host;
    EXPECT_NO_THROW(host.Shutdown());
    EXPECT_FALSE(host.IsInitialized());
}

// ============================================================================
// CoreCLRHost — Initialize With NonExistent Directory
// ============================================================================
TEST(CoreCLRHostTest, InitWithInvalidDirectory) {
    CoreCLRHost host;
    bool result = host.Initialize("/tmp/nonexistent_scripts_dir_xyz");
    EXPECT_FALSE(result);
    EXPECT_FALSE(host.IsInitialized());
}

// ============================================================================
// CoreCLRHost — GetFunctionPointer Without Init
// ============================================================================
TEST(CoreCLRHostTest, GetFunctionPointerReturnsNullBeforeInit) {
    CoreCLRHost host;
    void* fn = host.GetFunctionPointer("test.dll", "Test.Type", "TestMethod");
    EXPECT_EQ(fn, nullptr);
}

// ============================================================================
// CoreCLRHost — Copy Disabled
// ============================================================================
TEST(CoreCLRHostTest, CopyDisabled) {
    EXPECT_TRUE(std::is_copy_constructible_v<CoreCLRHost> == false);
    EXPECT_TRUE(std::is_copy_assignable_v<CoreCLRHost> == false);
}

} // namespace
} // namespace Prisma::Scripting
