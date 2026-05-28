#include <gtest/gtest.h>
#include "core/Timestep.h"
#include <cmath>
#include <limits>

namespace Prisma {
namespace {

// Default construction yields zero time
TEST(TimestepTest, DefaultConstructIsZero) {
    Timestep ts;
    EXPECT_FLOAT_EQ(ts.GetSeconds(), 0.0f);
    EXPECT_FLOAT_EQ(ts.GetMilliseconds(), 0.0f);
}

// Explicit value construction
TEST(TimestepTest, ExplicitConstruction) {
    Timestep ts(2.5f);
    EXPECT_FLOAT_EQ(ts.GetSeconds(), 2.5f);
}

// GetMilliseconds is GetSeconds * 1000
TEST(TimestepTest, GetMillisecondsConversion) {
    Timestep ts(1.0f);
    EXPECT_FLOAT_EQ(ts.GetMilliseconds(), 1000.0f);

    Timestep ts2(0.5f);
    EXPECT_FLOAT_EQ(ts2.GetMilliseconds(), 500.0f);
}

// Operator float() returns seconds
TEST(TimestepTest, FloatConversion) {
    Timestep ts(3.14159f);
    float val = static_cast<float>(ts);
    EXPECT_FLOAT_EQ(val, 3.14159f);
}

// Implicit float conversion works for arithmetic
TEST(TimestepTest, ImplicitFloatInArithmetic) {
    Timestep ts(2.0f);
    float doubled = ts * 2.0f;
    EXPECT_FLOAT_EQ(doubled, 4.0f);
}

// Negative timestep (should be allowed, e.g. for time rewinding)
TEST(TimestepTest, NegativeTimestep) {
    Timestep ts(-1.5f);
    EXPECT_FLOAT_EQ(ts.GetSeconds(), -1.5f);
    EXPECT_FLOAT_EQ(ts.GetMilliseconds(), -1500.0f);
}

// Very small timestep (floating point precision)
TEST(TimestepTest, VerySmallTimestep) {
    float epsilon = std::numeric_limits<float>::epsilon();
    Timestep ts(epsilon);
    EXPECT_FLOAT_EQ(ts.GetSeconds(), epsilon);
    EXPECT_FLOAT_EQ(ts.GetMilliseconds(), epsilon * 1000.0f);
}

// Very large timestep
TEST(TimestepTest, VeryLargeTimestep) {
    float large = 1e6f; // 1 million seconds
    Timestep ts(large);
    EXPECT_FLOAT_EQ(ts.GetMilliseconds(), large * 1000.0f);
}

// Zero timestep
TEST(TimestepTest, ZeroTimestep) {
    Timestep ts(0.0f);
    EXPECT_FLOAT_EQ(ts.GetSeconds(), 0.0f);
    EXPECT_FLOAT_EQ(ts.GetMilliseconds(), 0.0f);
}

} // namespace
} // namespace Prisma
