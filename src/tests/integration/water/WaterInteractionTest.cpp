#include <gtest/gtest.h>
#include "water/WaterInteraction.h"

namespace Prisma {
namespace Water {
namespace {

// ============================================================================
// WaterInteraction — Default State
// ============================================================================
TEST(WaterInteractionTest, DefaultConstructor) {
    WaterInteraction wi;
    EXPECT_EQ(wi.GetActiveCount(), 0u);
    EXPECT_TRUE(wi.GetRipples().empty());
    EXPECT_FLOAT_EQ(wi.GetDisplacement(Vector2(0.0f, 0.0f)), 0.0f);
}

// ============================================================================
// WaterInteraction — SpawnRipple
// ============================================================================
TEST(WaterInteractionTest, SpawnRippleBasic) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(wi.GetActiveCount(), 1u);
    EXPECT_FALSE(wi.GetRipples().empty());
}

TEST(WaterInteractionTest, SpawnRippleCustomParams) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(10.0f, 0.0f, -5.0f), 2.0f, 20.0f, 5.0f);
    ASSERT_EQ(wi.GetActiveCount(), 1u);

    const auto& ripples = wi.GetRipples();
    const auto& r = ripples[0];
    EXPECT_FLOAT_EQ(r.center.x, 10.0f);
    EXPECT_FLOAT_EQ(r.center.y, 0.0f);
    EXPECT_FLOAT_EQ(r.center.z, -5.0f);
    EXPECT_FLOAT_EQ(r.maxRadius, 20.0f);
    EXPECT_FLOAT_EQ(r.lifetime, 5.0f);
    EXPECT_FLOAT_EQ(r.age, 0.0f);
    EXPECT_FLOAT_EQ(r.currentRadius, 0.0f);
    EXPECT_TRUE(r.IsAlive());
}

TEST(WaterInteractionTest, SpawnRippleMultiple) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f));
    wi.SpawnRipple(Vector3(1.0f, 0.0f, 1.0f));
    wi.SpawnRipple(Vector3(2.0f, 0.0f, 2.0f));
    EXPECT_EQ(wi.GetActiveCount(), 3u);
}

// ============================================================================
// WaterInteraction — Ripple Properties (Ripple struct)
// ============================================================================
TEST(WaterInteractionTest, RippleIsAlive) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 4.0f);
    auto& ripples = const_cast<std::vector<Ripple>&>(wi.GetRipples());
    EXPECT_TRUE(ripples[0].IsAlive());
}

TEST(WaterInteractionTest, RippleGetNormalizedAge) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 4.0f);
    const auto& r = wi.GetRipples()[0];
    EXPECT_FLOAT_EQ(r.GetNormalizedAge(), 0.0f);
    EXPECT_FLOAT_EQ(r.GetFade(), 1.0f);
}

// ============================================================================
// WaterInteraction — Update
// ============================================================================
TEST(WaterInteractionTest, UpdateAdvancesAge) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 4.0f);
    wi.Update(1.0f, 5.0f, 0.5f);
    ASSERT_EQ(wi.GetActiveCount(), 1u);
    const auto& r = wi.GetRipples()[0];
    EXPECT_FLOAT_EQ(r.age, 1.0f);
    EXPECT_FLOAT_EQ(r.currentRadius, 5.0f);
    EXPECT_TRUE(r.IsAlive());
}

TEST(WaterInteractionTest, UpdateExpandsRadius) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 50.0f, 10.0f);
    wi.Update(2.0f, 10.0f, 0.1f);
    ASSERT_EQ(wi.GetActiveCount(), 1u);
    // After 2s at speed 10 m/s: radius should be 20
    EXPECT_FLOAT_EQ(wi.GetRipples()[0].currentRadius, 20.0f);
    EXPECT_FLOAT_EQ(wi.GetRipples()[0].age, 2.0f);
}

TEST(WaterInteractionTest, UpdateRippleExpires) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 1.0f);
    wi.Update(1.0f, 5.0f, 0.1f);
    // After lifetime (1.0s), ripple should be removed
    EXPECT_EQ(wi.GetActiveCount(), 0u);
}

TEST(WaterInteractionTest, UpdateRippleFullyExpires) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 2.0f);
    wi.Update(3.0f, 5.0f, 0.1f);
    EXPECT_EQ(wi.GetActiveCount(), 0u);
}

TEST(WaterInteractionTest, UpdateDecayAmplitude) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 4.0f);
    wi.Update(1.0f, 5.0f, 0.5f);
    ASSERT_EQ(wi.GetActiveCount(), 1u);
    EXPECT_NEAR(wi.GetRipples()[0].amplitude, 0.5f, 0.001f);
}

// ============================================================================
// WaterInteraction — GetDisplacement
// ============================================================================
TEST(WaterInteractionTest, GetDisplacementNoRipples) {
    WaterInteraction wi;
    EXPECT_FLOAT_EQ(wi.GetDisplacement(Vector2(0.0f, 0.0f)), 0.0f);
    EXPECT_FLOAT_EQ(wi.GetDisplacement(Vector2(100.0f, 100.0f)), 0.0f);
}

TEST(WaterInteractionTest, GetDisplacementAtRippleRing) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 4.0f);
    wi.Update(1.0f, 5.0f, 0.1f);
    EXPECT_EQ(wi.GetActiveCount(), 1u);
    // Displacement at center is near-zero (floating point precision)
    EXPECT_NEAR(wi.GetDisplacement(Vector2(0.0f, 0.0f)), 0.0f, 1e-6f);
    // At a point slightly offset from the ring (where sin(k*r - k*R) ≠ 0)
    float dispOffset = wi.GetDisplacement(Vector2(5.5f, 0.0f));
    EXPECT_NE(dispOffset, 0.0f);
}

TEST(WaterInteractionTest, GetDisplacementAwayFromRipple) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 4.0f);
    wi.Update(1.0f, 5.0f, 0.1f);
    ASSERT_EQ(wi.GetActiveCount(), 1u);
    float dispAtCenter = wi.GetDisplacement(Vector2(0.0f, 0.0f));
    float dispFar = wi.GetDisplacement(Vector2(100.0f, 100.0f));
    EXPECT_NE(dispAtCenter, 0.0f);
    EXPECT_NEAR(dispFar, 0.0f, 0.01f);
}

TEST(WaterInteractionTest, GetDisplacementAfterRippleExpires) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 0.5f);
    wi.Update(1.0f, 5.0f, 0.1f);
    EXPECT_EQ(wi.GetActiveCount(), 0u);
    float disp = wi.GetDisplacement(Vector2(0.0f, 0.0f));
    EXPECT_FLOAT_EQ(disp, 0.0f);
}

// ============================================================================
// WaterInteraction — Clear
// ============================================================================
TEST(WaterInteractionTest, ClearEmpty) {
    WaterInteraction wi;
    wi.Clear();
    EXPECT_EQ(wi.GetActiveCount(), 0u);
}

TEST(WaterInteractionTest, ClearWithRipples) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f));
    wi.SpawnRipple(Vector3(1.0f, 0.0f, 1.0f));
    EXPECT_EQ(wi.GetActiveCount(), 2u);
    wi.Clear();
    EXPECT_EQ(wi.GetActiveCount(), 0u);
    EXPECT_TRUE(wi.GetRipples().empty());
}

// ============================================================================
// WaterInteraction — SetMaxRipples
// ============================================================================
TEST(WaterInteractionTest, SetMaxRipples) {
    WaterInteraction wi;
    wi.SetMaxRipples(2);
    // Spawn 3 ripples, only 2 should be active
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f));
    wi.SpawnRipple(Vector3(1.0f, 0.0f, 0.0f));
    wi.SpawnRipple(Vector3(2.0f, 0.0f, 0.0f));
    EXPECT_LE(wi.GetActiveCount(), 2u);
}

TEST(WaterInteractionTest, SetMaxRipplesDefault) {
    WaterInteraction wi;
    // Default max is 64
    for (int i = 0; i < 100; ++i) {
        wi.SpawnRipple(Vector3(static_cast<float>(i), 0.0f, 0.0f));
    }
    EXPECT_LE(wi.GetActiveCount(), 64u);
}

// ============================================================================
// WaterInteraction — Multiple Ripple Interaction
// ============================================================================
TEST(WaterInteractionTest, MultipleRipplesDisplacement) {
    WaterInteraction wi;
    // Spawn two ripples
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 4.0f);
    wi.SpawnRipple(Vector3(5.0f, 0.0f, 5.0f), 1.0f, 10.0f, 4.0f);
    EXPECT_EQ(wi.GetActiveCount(), 2u);

    // Displacement should be sum of both ripples
    float disp = wi.GetDisplacement(Vector2(0.0f, 0.0f));
    EXPECT_NE(disp, 0.0f);
}

TEST(WaterInteractionTest, UpdateMultipleRipples) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 2.0f);
    wi.SpawnRipple(Vector3(5.0f, 0.0f, 0.0f), 2.0f, 15.0f, 3.0f);
    wi.Update(1.0f, 5.0f, 0.5f);
    ASSERT_EQ(wi.GetActiveCount(), 2u);
    for (const auto& r : wi.GetRipples()) {
        EXPECT_FLOAT_EQ(r.age, 1.0f);
        EXPECT_TRUE(r.IsAlive());
    }
}

// ============================================================================
// WaterInteraction — Edge Cases
// ============================================================================
TEST(WaterInteractionTest, ZeroStrengthRipple) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 0.0f, 10.0f, 4.0f);
    EXPECT_EQ(wi.GetActiveCount(), 1u);
    // Zero strength ripple may or may not produce displacement
    float disp = wi.GetDisplacement(Vector2(0.0f, 0.0f));
    EXPECT_FLOAT_EQ(disp, 0.0f);
}

TEST(WaterInteractionTest, ZeroLifetimeRipple) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 0.0f);
    // With lifetime=0, the ripple should immediately be dead
    if (!wi.GetRipples().empty()) {
        EXPECT_FALSE(wi.GetRipples()[0].IsAlive());
    }
}

TEST(WaterInteractionTest, UpdateZeroDeltaTime) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 4.0f);
    wi.Update(0.0f, 5.0f, 1.0f);
    const auto& r = wi.GetRipples()[0];
    EXPECT_FLOAT_EQ(r.age, 0.0f);
    EXPECT_FLOAT_EQ(r.currentRadius, 0.0f);
    EXPECT_TRUE(r.IsAlive());
}

TEST(WaterInteractionTest, SpawnThenClearThenSpawn) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(wi.GetActiveCount(), 1u);
    wi.Clear();
    EXPECT_EQ(wi.GetActiveCount(), 0u);
    wi.SpawnRipple(Vector3(1.0f, 0.0f, 1.0f));
    EXPECT_EQ(wi.GetActiveCount(), 1u);
}

TEST(WaterInteractionTest, RippleGetFadeDecreases) {
    WaterInteraction wi;
    wi.SpawnRipple(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 10.0f, 10.0f);
    ASSERT_EQ(wi.GetActiveCount(), 1u);
    EXPECT_FLOAT_EQ(wi.GetRipples()[0].GetFade(), 1.0f);
    wi.Update(2.0f, 5.0f, 0.1f);
    ASSERT_EQ(wi.GetActiveCount(), 1u);
    EXPECT_FLOAT_EQ(wi.GetRipples()[0].GetFade(), 0.8f);
    wi.Update(5.0f, 5.0f, 0.1f);
    ASSERT_EQ(wi.GetActiveCount(), 1u);
    EXPECT_FLOAT_EQ(wi.GetRipples()[0].GetFade(), 0.3f);
}

} // namespace
} // namespace Water
} // namespace Prisma
