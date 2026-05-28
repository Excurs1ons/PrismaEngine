#include <gtest/gtest.h>
#include "input/InputDevice.h"

namespace Prisma {
namespace {

class InputDeviceTest : public ::testing::Test {
protected:
    Input::InputDevice m_Device;
};

TEST_F(InputDeviceTest, NotInitializedByDefault) {
    EXPECT_FALSE(m_Device.IsInitialized());
}

TEST_F(InputDeviceTest, KeyQueriesReturnFalseWhenNotInitialized) {
    EXPECT_FALSE(m_Device.IsKeyDown(Input::KeyCode::W));
    EXPECT_FALSE(m_Device.IsKeyJustPressed(Input::KeyCode::W));
    EXPECT_FALSE(m_Device.IsKeyJustReleased(Input::KeyCode::W));
    EXPECT_FALSE(m_Device.IsAnyKeyDown());
}

TEST_F(InputDeviceTest, MouseQueriesReturnSafeDefaults) {
    int x = -1, y = -1;
    m_Device.GetMousePosition(x, y);
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);

    int dx = -1, dy = -1;
    m_Device.GetMouseDelta(dx, dy);
    EXPECT_EQ(dx, 0);
    EXPECT_EQ(dy, 0);

    EXPECT_FALSE(m_Device.IsMouseButtonDown(Input::MouseButton::Left));
    EXPECT_FALSE(m_Device.IsMouseButtonJustPressed(Input::MouseButton::Left));
    EXPECT_FALSE(m_Device.IsMouseButtonJustReleased(Input::MouseButton::Left));
    EXPECT_EQ(m_Device.GetMouseWheelDelta(), 0);
}

TEST_F(InputDeviceTest, DeviceEnumerationWhenNotInitialized) {
    EXPECT_EQ(m_Device.GetGamepadCount(), 0u);
    EXPECT_FALSE(m_Device.IsGamepadConnected(0));
    EXPECT_FALSE(m_Device.IsGamepadButtonDown(0, Input::GamepadButton::A));
    EXPECT_FLOAT_EQ(m_Device.GetGamepadAxis(0, Input::GamepadAxis::LeftX), 0.0f);
}

TEST_F(InputDeviceTest, ActionMappingAddAndQuery) {
    m_Device.AddActionMapping("jump", Input::KeyCode::Space);
    m_Device.AddActionMapping("move_forward", Input::KeyCode::W, Input::KeyCode::Up);

    EXPECT_FALSE(m_Device.IsActionPressed("jump"));
    EXPECT_FALSE(m_Device.IsActionJustPressed("jump"));
}

TEST_F(InputDeviceTest, ActionMappingWithGamepad) {
    m_Device.AddActionMapping("fire", Input::GamepadButton::RightTrigger);
    EXPECT_FALSE(m_Device.IsActionPressed("fire"));
}

TEST_F(InputDeviceTest, UnknownActionReturnsFalse) {
    EXPECT_FALSE(m_Device.IsActionPressed("nonexistent_action"));
    EXPECT_FALSE(m_Device.IsActionJustPressed("nonexistent_action"));
}

TEST_F(InputDeviceTest, CursorVisibility) {
    EXPECT_FALSE(m_Device.IsCursorLocked());

    m_Device.SetCursorVisible(true);
    m_Device.SetCursorVisible(false);

    m_Device.SetCursorLocked(true);
    EXPECT_TRUE(m_Device.IsCursorLocked());

    m_Device.SetCursorLocked(false);
    EXPECT_FALSE(m_Device.IsCursorLocked());
}

TEST_F(InputDeviceTest, TextInputFlow) {
    EXPECT_TRUE(m_Device.GetTextInput().empty());

    m_Device.StartTextInput();
    m_Device.StopTextInput();
}

TEST_F(InputDeviceTest, VibrationControl) {
    m_Device.SetGamepadVibration(0, 0.5f, 0.5f, 1000);
}

} // namespace
} // namespace Prisma
