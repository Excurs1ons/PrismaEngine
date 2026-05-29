#include <gtest/gtest.h>
#include "graphic/pipelines/forward/TransparentPass.h"
#include "graphic/pipelines/forward/OpaquePass.h"
#include "graphic/interfaces/IPass.h"

namespace Prisma::Graphic {
namespace {

// ============================================================================
// TransparentPass — Setup & Default State
// ============================================================================
TEST(DISABLED_TestTransparentPassSetup, DefaultDepthSettings) {
    TransparentPass pass;

    EXPECT_TRUE(pass.GetDepthTest()) << "Transparent pass should enable depth test by default";
    EXPECT_FALSE(pass.GetDepthWrite()) << "Transparent pass should disable depth write by default";

    const auto& stats = pass.GetRenderStats();
    EXPECT_EQ(stats.drawCalls, 0u);
    EXPECT_EQ(stats.triangles, 0u);
    EXPECT_EQ(stats.transparentObjects, 0u);
}

// ============================================================================
// TransparentPass — Depth State Mutators
// ============================================================================
TEST(DISABLED_TestTransparentPassAlphaBlending, DepthStateToggle) {
    TransparentPass pass;

    pass.SetDepthWrite(true);
    EXPECT_TRUE(pass.GetDepthWrite());

    pass.SetDepthWrite(false);
    EXPECT_FALSE(pass.GetDepthWrite());

    pass.SetDepthTest(false);
    EXPECT_FALSE(pass.GetDepthTest());

    pass.SetDepthTest(true);
    EXPECT_TRUE(pass.GetDepthTest());
}

// ============================================================================
// TransparentPass — Render Stats Lifecycle
// ============================================================================
TEST(DISABLED_TestTransparentPassRender, StatsReset) {
    TransparentPass pass;

    auto& stats = pass.GetRenderStats();
    stats.drawCalls = 42;
    stats.triangles = 1234;
    stats.transparentObjects = 10;

    pass.ResetStats();
    const auto& resetStats = pass.GetRenderStats();
    EXPECT_EQ(resetStats.drawCalls, 0u);
    EXPECT_EQ(resetStats.triangles, 0u);
    EXPECT_EQ(resetStats.transparentObjects, 0u);

    pass.ResetStats();
    const auto& idleStats = pass.GetRenderStats();
    EXPECT_EQ(idleStats.drawCalls, 0u);
}

// ============================================================================
// OpaquePass — Default Construction
// ============================================================================
TEST(DISABLED_TestOpaquePassRender, DefaultPipelineMode) {
    OpaquePass pass;

    EXPECT_FALSE(pass.IsUsingOffscreenPipeline())
        << "OpaquePass should default to swapchain pipeline mode";

    pass.SetUseOffscreenPipeline(true);
    EXPECT_TRUE(pass.IsUsingOffscreenPipeline());

    pass.SetUseOffscreenPipeline(false);
    EXPECT_FALSE(pass.IsUsingOffscreenPipeline());
}

// ============================================================================
// OpaquePass — Pass Identity
// ============================================================================
TEST(DISABLED_TestOpaquePassRender, PassIdentity) {
    OpaquePass pass;

    EXPECT_NE(pass.GetName(), nullptr);
    EXPECT_STRNE(pass.GetName(), "");

    EXPECT_TRUE(pass.IsEnabled());

    pass.SetEnabled(false);
    EXPECT_FALSE(pass.IsEnabled());

    pass.SetEnabled(true);
    EXPECT_TRUE(pass.IsEnabled());
}

} // namespace
} // namespace Prisma::Graphic
