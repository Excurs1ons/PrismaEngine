#include <gtest/gtest.h>
#include "graphic/pipelines/forward/BloomPostProcessPass.h"
#include "graphic/interfaces/IRenderDevice.h"

namespace Prisma::Graphic {
namespace {

// ============================================================================
// BloomPostProcessPass — Prefilter Parameters
// ============================================================================
TEST(DISABLED_TestBloomPrefilter, ThresholdParameter) {
    BloomPostProcessPass bloom;

    EXPECT_FLOAT_EQ(bloom.GetThreshold(), 1.0f)
        << "Default bloom threshold should be 1.0";

    bloom.SetThreshold(0.5f);
    EXPECT_FLOAT_EQ(bloom.GetThreshold(), 0.5f);

    bloom.SetThreshold(2.0f);
    EXPECT_FLOAT_EQ(bloom.GetThreshold(), 2.0f);

    bloom.SetThreshold(0.0f);
    EXPECT_FLOAT_EQ(bloom.GetThreshold(), 0.0f);
}

// ============================================================================
// BloomPostProcessPass — Blur Parameters
// ============================================================================
TEST(DISABLED_TestBloomBlur, BlurRadiusAndIterations) {
    BloomPostProcessPass bloom;

    EXPECT_FLOAT_EQ(bloom.GetBlurRadius(), 1.0f)
        << "Default Kawase blur radius should be 1.0";

    EXPECT_EQ(bloom.GetBlurIterations(), 3)
        << "Default blur iterations should be 3";

    bloom.SetBlurRadius(2.5f);
    EXPECT_FLOAT_EQ(bloom.GetBlurRadius(), 2.5f);

    bloom.SetBlurIterations(5);
    EXPECT_EQ(bloom.GetBlurIterations(), 5);

    bloom.SetBlurIterations(0);
    EXPECT_EQ(bloom.GetBlurIterations(), 0);

    bloom.SetBlurRadius(100.0f);
    EXPECT_FLOAT_EQ(bloom.GetBlurRadius(), 100.0f);
}

// ============================================================================
// BloomPostProcessPass — Readiness State
// ============================================================================
TEST(DISABLED_TestBloomComposite, ReadinessState) {
    BloomPostProcessPass bloom;

    EXPECT_FALSE(bloom.IsReady())
        << "Bloom should not be ready before Setup";

    bloom.SetThreshold(0.8f);
    bloom.SetBlurRadius(1.5f);
    bloom.SetBlurIterations(4);
    EXPECT_FALSE(bloom.IsReady())
        << "Parameter changes alone should not make bloom ready";
}

} // namespace
} // namespace Prisma::Graphic
