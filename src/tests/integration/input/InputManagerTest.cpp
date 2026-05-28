#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "input/InputManager.h"

namespace Prisma {
namespace {

// MockIInputDriver does not inherit from real IInputDriver due to
// conflicting KeyCode enum definitions in the same namespace.
class MockIInputDriver {
public:
    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Shutdown, (), ());
    MOCK_METHOD(bool, IsInitialized, (), (const));
    MOCK_METHOD(void, Update, (), ());
    MOCK_METHOD(bool, IsKeyDown, (Input::KeyCode), (const));
    MOCK_METHOD(bool, IsKeyJustPressed, (Input::KeyCode), (const));
    MOCK_METHOD(bool, IsKeyJustReleased, (Input::KeyCode), (const));
};

class InputManagerTest : public ::testing::Test {
protected:
    Input::InputManager m_Manager;

    void SetUp() override {
        m_Manager.Initialize();
    }

    void TearDown() override {
        m_Manager.Shutdown();
    }
};

TEST_F(InputManagerTest, KeyPressedState) {
    EXPECT_FALSE(m_Manager.IsKeyPressed(Input::KeyCode::W));
    EXPECT_FALSE(m_Manager.IsKeyJustPressed(Input::KeyCode::W));

    m_Manager.SetKeyState(Input::KeyCode::W, true);

    EXPECT_TRUE(m_Manager.IsKeyPressed(Input::KeyCode::W));
    EXPECT_TRUE(m_Manager.IsKeyJustPressed(Input::KeyCode::W));
}

TEST_F(InputManagerTest, KeyReleasedState) {
    m_Manager.SetKeyState(Input::KeyCode::Space, true);
    EXPECT_TRUE(m_Manager.IsKeyPressed(Input::KeyCode::Space));

    m_Manager.SetKeyState(Input::KeyCode::Space, false);

    EXPECT_FALSE(m_Manager.IsKeyPressed(Input::KeyCode::Space));
    EXPECT_TRUE(m_Manager.IsKeyJustReleased(Input::KeyCode::Space));
}

TEST_F(InputManagerTest, UpdateClearsJustStates) {
    m_Manager.SetKeyState(Input::KeyCode::A, true);
    m_Manager.SetKeyState(Input::KeyCode::C, true);
    m_Manager.SetKeyState(Input::KeyCode::C, false);

    EXPECT_TRUE(m_Manager.IsKeyJustPressed(Input::KeyCode::A));
    EXPECT_TRUE(m_Manager.IsKeyJustReleased(Input::KeyCode::C));

    m_Manager.Update(Timestep{1.0f});

    EXPECT_FALSE(m_Manager.IsKeyJustPressed(Input::KeyCode::A));
    EXPECT_FALSE(m_Manager.IsKeyJustReleased(Input::KeyCode::C));
    EXPECT_TRUE(m_Manager.IsKeyPressed(Input::KeyCode::A));
    EXPECT_FALSE(m_Manager.IsKeyPressed(Input::KeyCode::C));
}

TEST_F(InputManagerTest, MouseButtonState) {
    EXPECT_FALSE(m_Manager.IsMouseButtonPressed(Input::MouseButton::Left));
    EXPECT_FALSE(m_Manager.IsMouseButtonJustPressed(Input::MouseButton::Left));

    m_Manager.SetMouseButtonState(Input::MouseButton::Left, true);

    EXPECT_TRUE(m_Manager.IsMouseButtonPressed(Input::MouseButton::Left));
    EXPECT_TRUE(m_Manager.IsMouseButtonJustPressed(Input::MouseButton::Left));

    m_Manager.SetMouseButtonState(Input::MouseButton::Left, false);

    EXPECT_FALSE(m_Manager.IsMouseButtonPressed(Input::MouseButton::Left));
    EXPECT_TRUE(m_Manager.IsMouseButtonJustReleased(Input::MouseButton::Left));
}

TEST_F(InputManagerTest, MousePosition) {
    PrismaMath::vec2 pos{100.0f, 200.0f};
    m_Manager.SetMousePosition(pos);

    auto result = m_Manager.GetMousePosition();
    EXPECT_FLOAT_EQ(result.x, 100.0f);
    EXPECT_FLOAT_EQ(result.y, 200.0f);
}

TEST_F(InputManagerTest, ClearStateResetsAll) {
    m_Manager.SetKeyState(Input::KeyCode::W, true);
    m_Manager.SetMouseButtonState(Input::MouseButton::Right, true);
    m_Manager.SetMousePosition(PrismaMath::vec2{50.0f, 75.0f});

    m_Manager.ClearState();

    EXPECT_FALSE(m_Manager.IsKeyPressed(Input::KeyCode::W));
    EXPECT_FALSE(m_Manager.IsKeyJustPressed(Input::KeyCode::W));
    EXPECT_FALSE(m_Manager.IsMouseButtonPressed(Input::MouseButton::Right));
    EXPECT_FALSE(m_Manager.IsMouseButtonJustPressed(Input::MouseButton::Right));
}

TEST_F(InputManagerTest, GetName) {
    EXPECT_STREQ(m_Manager.GetName(), "InputManager");
}

TEST_F(InputManagerTest, SetKeyStateIdempotent) {
    m_Manager.SetKeyState(Input::KeyCode::A, true);
    EXPECT_TRUE(m_Manager.IsKeyJustPressed(Input::KeyCode::A));

    m_Manager.SetKeyState(Input::KeyCode::A, true);
    EXPECT_TRUE(m_Manager.IsKeyJustPressed(Input::KeyCode::A));
}

TEST_F(InputManagerTest, MultipleKeysSimultaneously) {
    m_Manager.SetKeyState(Input::KeyCode::W, true);
    m_Manager.SetKeyState(Input::KeyCode::A, true);
    m_Manager.SetKeyState(Input::KeyCode::S, true);
    m_Manager.SetKeyState(Input::KeyCode::D, true);

    EXPECT_TRUE(m_Manager.IsKeyPressed(Input::KeyCode::W));
    EXPECT_TRUE(m_Manager.IsKeyPressed(Input::KeyCode::A));
    EXPECT_TRUE(m_Manager.IsKeyPressed(Input::KeyCode::S));
    EXPECT_TRUE(m_Manager.IsKeyPressed(Input::KeyCode::D));
}

TEST_F(InputManagerTest, MouseDeltaComputedOnUpdate) {
    m_Manager.SetMousePosition(PrismaMath::vec2{10.0f, 20.0f});
    m_Manager.Update(Timestep{1.0f});

    auto delta1 = m_Manager.GetMouseDelta();
    EXPECT_FLOAT_EQ(delta1.x, 10.0f);
    EXPECT_FLOAT_EQ(delta1.y, 20.0f);

    m_Manager.SetMousePosition(PrismaMath::vec2{30.0f, 50.0f});
    m_Manager.Update(Timestep{1.0f});

    auto delta2 = m_Manager.GetMouseDelta();
    EXPECT_FLOAT_EQ(delta2.x, 20.0f);
    EXPECT_FLOAT_EQ(delta2.y, 30.0f);
}

} // namespace
} // namespace Prisma
