#include <gtest/gtest.h>
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "core/EntityManager.h"
#include "core/Timestep.h"

namespace Prisma {
namespace {

// ============================================================================
// SceneManager Test Fixture
// ============================================================================
class SceneManagerTest : public ::testing::Test {
protected:
    EntityManager m_em;
    SceneManager m_manager;

    void SetUp() override {
        m_manager.Initialize();
    }

    void TearDown() override {
        m_manager.Shutdown();
    }
};

// ============================================================================
// SceneManager — Default State
// ============================================================================
TEST_F(SceneManagerTest, DefaultState) {
    EXPECT_STREQ(m_manager.GetName(), "SceneManager");
    EXPECT_EQ(m_manager.GetCurrentScene(), nullptr);
}

// ============================================================================
// SceneManager — Create New Scene
// ============================================================================
TEST_F(SceneManagerTest, CreateNewScene) {
    m_manager.CreateNewScene();

    Scene* scene = m_manager.GetCurrentScene();
    ASSERT_NE(scene, nullptr);
    EXPECT_FALSE(scene->GetName().empty());
}

TEST_F(SceneManagerTest, CreateNewSceneHasMainCamera) {
    m_manager.CreateNewScene();

    Scene* scene = m_manager.GetCurrentScene();
    ASSERT_NE(scene, nullptr);

    auto camera = scene->GetMainCamera();
    EXPECT_NE(camera, nullptr);
}

// ============================================================================
// SceneManager — Multiple Scene Creation
// ============================================================================
TEST_F(SceneManagerTest, CreateMultipleScenesReplaceOld) {
    m_manager.CreateNewScene();
    Scene* first = m_manager.GetCurrentScene();
    ASSERT_NE(first, nullptr);

    m_manager.CreateNewScene();
    Scene* second = m_manager.GetCurrentScene();
    ASSERT_NE(second, nullptr);

    // The new scene should be different from the old one
    EXPECT_NE(first, second);
}

// ============================================================================
// SceneManager — Update delegates to current scene
// ============================================================================
TEST_F(SceneManagerTest, UpdateWithNoSceneDoesNotCrash) {
    EXPECT_EQ(m_manager.GetCurrentScene(), nullptr);
    // Should not crash
    m_manager.Update(Timestep(1.0f / 60.0f));
}

TEST_F(SceneManagerTest, UpdateWithSceneDoesNotCrash) {
    m_manager.CreateNewScene();
    // Should not crash
    m_manager.Update(Timestep(1.0f / 60.0f));
}

// ============================================================================
// SceneManager — Shutdown clears scene
// ============================================================================
TEST_F(SceneManagerTest, ShutdownClearsScene) {
    m_manager.CreateNewScene();
    ASSERT_NE(m_manager.GetCurrentScene(), nullptr);

    m_manager.Shutdown();
    EXPECT_EQ(m_manager.GetCurrentScene(), nullptr);
}

} // namespace
} // namespace Prisma
