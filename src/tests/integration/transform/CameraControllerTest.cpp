#include <gtest/gtest.h>
#include "transform/CameraController.h"

namespace Prisma {
namespace {

// ============================================================================
// CameraController — Construction and Defaults
// ============================================================================
TEST(CameraControllerTest, DefaultConstruction) {
    CameraController ctrl;

    EXPECT_FLOAT_EQ(ctrl.GetMoveSpeed(), 5.0f);
    EXPECT_FLOAT_EQ(ctrl.GetRotationSpeed(), 90.0f);
    EXPECT_FALSE(ctrl.GetMouseControl());  // default false per constructor
}

// ============================================================================
// CameraController — Move Speed
// ============================================================================
TEST(CameraControllerTest, SetMoveSpeed) {
    CameraController ctrl;

    ctrl.SetMoveSpeed(10.0f);
    EXPECT_FLOAT_EQ(ctrl.GetMoveSpeed(), 10.0f);

    ctrl.SetMoveSpeed(0.0f);
    EXPECT_FLOAT_EQ(ctrl.GetMoveSpeed(), 0.0f);

    ctrl.SetMoveSpeed(-5.0f);
    EXPECT_FLOAT_EQ(ctrl.GetMoveSpeed(), -5.0f);
}

// ============================================================================
// CameraController — Rotation Speed
// ============================================================================
TEST(CameraControllerTest, SetRotationSpeed) {
    CameraController ctrl;

    ctrl.SetRotationSpeed(45.0f);
    EXPECT_FLOAT_EQ(ctrl.GetRotationSpeed(), 45.0f);

    ctrl.SetRotationSpeed(180.0f);
    EXPECT_FLOAT_EQ(ctrl.GetRotationSpeed(), 180.0f);

    ctrl.SetRotationSpeed(0.0f);
    EXPECT_FLOAT_EQ(ctrl.GetRotationSpeed(), 0.0f);
}

// ============================================================================
// CameraController — Mouse Control Toggle
// ============================================================================
TEST(CameraControllerTest, SetMouseControl) {
    CameraController ctrl;

    EXPECT_FALSE(ctrl.GetMouseControl());  // default

    ctrl.SetMouseControl(true);
    EXPECT_TRUE(ctrl.GetMouseControl());

    ctrl.SetMouseControl(false);
    EXPECT_FALSE(ctrl.GetMouseControl());
}

} // namespace
} // namespace Prisma
