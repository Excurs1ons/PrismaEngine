#include <gtest/gtest.h>
#include "transform/Camera.h"
#include "transform/Transform.h"
#include "scene/Scene.h"
#include "core/EntityManager.h"

namespace Prisma {
namespace {

// ============================================================================
// Camera — Default State
// ============================================================================
TEST(CameraTest, DefaultState) {
    Graphic::Camera cam;

    EXPECT_EQ(cam.GetProjectionMode(), Graphic::ProjectionMode::Perspective);
    // Camera default FOV is PI/4 = 45 degrees (radians)
    EXPECT_NEAR(cam.GetFOV(), Prisma::PI / 4.0f, 1e-5f);
    EXPECT_NEAR(cam.GetAspectRatio(), 16.0f / 9.0f, 1e-5f);
    EXPECT_NEAR(cam.GetNearPlane(), 0.1f, 1e-5f);
    EXPECT_NEAR(cam.GetFarPlane(), 1000.0f, 1e-5f);
    EXPECT_TRUE(cam.IsActive());
    EXPECT_FLOAT_EQ(cam.GetClearColor().r, 0.0f);
    EXPECT_FLOAT_EQ(cam.GetClearColor().g, 0.0f);
    EXPECT_FLOAT_EQ(cam.GetClearColor().b, 0.0f);
    EXPECT_FLOAT_EQ(cam.GetClearColor().a, 1.0f);
}

// ============================================================================
// Camera — Perspective Projection Matrix
// ============================================================================
TEST(CameraTest, PerspectiveProjectionMatrix) {
    Graphic::Camera cam;
    cam.SetPerspectiveProjection(glm::radians(70.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    Matrix4x4 proj = cam.GetProjectionMatrix();

    // Projection matrix should not be identity for perspective
    EXPECT_NE(proj, Matrix4x4(1.0f));

    // The [0][0] element for LH perspective: 1/(aspect * tan(fov/2))
    float expected00 = 1.0f / ((16.0f / 9.0f) * std::tan(glm::radians(70.0f) * 0.5f));
    EXPECT_NEAR(proj[0][0], expected00, 1e-5f);

    // The [1][1] element: 1/tan(fov/2), negated for Vulkan Y-flip
    float expected11 = -1.0f / std::tan(glm::radians(70.0f) * 0.5f);
    EXPECT_NEAR(proj[1][1], expected11, 1e-5f);
}

// ============================================================================
// Camera — Orthographic Projection Matrix
// ============================================================================
TEST(CameraTest, OrthographicProjectionMatrix) {
    Graphic::Camera cam;
    cam.SetOrthographicProjection(10.0f, 16.0f / 9.0f, 0.1f, 1000.0f);

    EXPECT_EQ(cam.GetProjectionMode(), Graphic::ProjectionMode::Orthographic);

    Matrix4x4 proj = cam.GetProjectionMatrix();

    // Ortho size 10 → halfH = 5, halfW = 5 * (16/9) = 8.8889
    const float halfH = 10.0f * 0.5f;
    const float halfW = halfH * (16.0f / 9.0f);

    const float expected00 = 2.0f / (2.0f * halfW);
    EXPECT_NEAR(proj[0][0], expected00, 1e-5f);

    const float expected11 = 2.0f / (2.0f * halfH);
    EXPECT_NEAR(proj[1][1], expected11, 1e-5f);
}

// ============================================================================
// Camera — Set Projection Parameters
// ============================================================================
TEST(CameraTest, SetFOV) {
    Graphic::Camera cam;
    cam.SetFOV(glm::radians(90.0f));
    EXPECT_NEAR(cam.GetFOV(), glm::radians(90.0f), 1e-5f);
}

TEST(CameraTest, SetAspectRatio) {
    Graphic::Camera cam;
    cam.SetAspectRatio(4.0f / 3.0f);
    EXPECT_NEAR(cam.GetAspectRatio(), 4.0f / 3.0f, 1e-5f);
}

TEST(CameraTest, SetNearFarPlanes) {
    Graphic::Camera cam;
    cam.SetNearFarPlanes(0.5f, 500.0f);
    EXPECT_NEAR(cam.GetNearPlane(), 0.5f, 1e-5f);
    EXPECT_NEAR(cam.GetFarPlane(), 500.0f, 1e-5f);
}

TEST(CameraTest, SetViewportUpdatesAspectRatio) {
    Graphic::Camera cam;
    cam.SetViewport(1920, 1080);
    EXPECT_NEAR(cam.GetAspectRatio(), 1920.0f / 1080.0f, 1e-5f);
}

TEST(CameraTest, SetClearColor) {
    Graphic::Camera cam;
    cam.SetClearColor(0.2f, 0.3f, 0.4f, 0.5f);
    Vector4 cc = cam.GetClearColor();
    EXPECT_FLOAT_EQ(cc.r, 0.2f);
    EXPECT_FLOAT_EQ(cc.g, 0.3f);
    EXPECT_FLOAT_EQ(cc.b, 0.4f);
    EXPECT_FLOAT_EQ(cc.a, 0.5f);
}

// ============================================================================
// Camera — Active State
// ============================================================================
TEST(CameraTest, ActiveState) {
    Graphic::Camera cam;
    EXPECT_TRUE(cam.IsActive());
    cam.SetActive(false);
    EXPECT_FALSE(cam.IsActive());
    cam.SetActive(true);
    EXPECT_TRUE(cam.IsActive());
}

// ============================================================================
// Camera — Parameter Boundaries
// ============================================================================
TEST(CameraTest, VerySmallFOV) {
    Graphic::Camera cam;

    // Very small FOV should still produce a valid projection matrix
    cam.SetFOV(glm::radians(1.0f));
    Matrix4x4 proj = cam.GetProjectionMatrix();

    // tan(0.5°) ≈ 0.00873; 00 element should reflect this
    float expected00 = 1.0f / ((16.0f / 9.0f) * std::tan(glm::radians(0.5f)));
    EXPECT_NEAR(proj[0][0], expected00, 1e-3f);
    EXPECT_TRUE(std::isfinite(proj[0][0]));
}

TEST(CameraTest, VeryLargeFOV) {
    Graphic::Camera cam;

    // Large FOV should not produce NaN
    cam.SetFOV(glm::radians(179.0f));
    Matrix4x4 proj = cam.GetProjectionMatrix();
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            EXPECT_TRUE(std::isfinite(proj[c][r]));
        }
    }
}

TEST(CameraTest, SwappedNearFarPlanes) {
    Graphic::Camera cam;

    // Camera::Update should swap near/far if far < near
    cam.SetNearFarPlanes(100.0f, 0.1f);  // intentionally reversed

    // Calling Update performs the swap
    cam.Update(Timestep(1.0f));

    EXPECT_NEAR(cam.GetNearPlane(), 0.1f, 1e-5f);
    EXPECT_NEAR(cam.GetFarPlane(), 100.0f, 1e-5f);
}

// ============================================================================
// Camera — LookAt with Scene Context
// ============================================================================
class CameraWithSceneTest : public ::testing::Test {
protected:
    EntityManager m_em;
    Scene m_scene;

    void SetUp() override {
        // Create a node with Transform + Camera
        auto node = m_scene.CreateNode("TestCamera");
        m_scene.AddComponent<Transform>(node);
        m_camera = m_scene.AddComponent<Graphic::Camera>(node);
    }

    std::shared_ptr<Graphic::Camera> m_camera;
};

TEST_F(CameraWithSceneTest, LookAtTarget) {
    // Camera at (0, 0, 10) looking at origin
    // Move the transform to (0, 0, 10)
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);
    transform->SetPosition(Vector3(0.0f, 0.0f, 10.0f));

    // Clear view dirty by getting view matrix first
    m_camera->LookAt(Vector3(0.0f, 0.0f, 0.0f));

    // After LookAt, the camera should be looking toward -Z (from +Z toward origin)
    Vector3 fwd = m_camera->GetForward();
    EXPECT_NEAR(fwd.x, 0.0f, 1e-5f);
    EXPECT_NEAR(fwd.y, 0.0f, 1e-5f);
    EXPECT_NEAR(fwd.z, -1.0f, 1e-5f);

    // View matrix should not be identity
    Matrix4x4 view = m_camera->GetViewMatrix();
    EXPECT_NE(view, Matrix4x4(1.0f));
}

TEST_F(CameraWithSceneTest, LookAtPosition) {
    // Camera at (5, 5, 5) looking at origin
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);
    transform->SetPosition(Vector3(5.0f, 5.0f, 5.0f));

    m_camera->LookAt(Vector3(0.0f, 0.0f, 0.0f));

    // Forward should point toward origin (from (5,5,5) toward (0,0,0))
    Vector3 expectedDir = glm::normalize(Vector3(-5.0f, -5.0f, -5.0f));
    Vector3 fwd = m_camera->GetForward();
    EXPECT_NEAR(fwd.x, expectedDir.x, 1e-5f);
    EXPECT_NEAR(fwd.y, expectedDir.y, 1e-5f);
    EXPECT_NEAR(fwd.z, expectedDir.z, 1e-5f);
}

TEST_F(CameraWithSceneTest, LookAtThenGetViewProjection) {
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);
    transform->SetPosition(Vector3(0.0f, 0.0f, 5.0f));

    m_camera->LookAt(Vector3(0.0f, 0.0f, 0.0f));

    // View-Projection should be a valid 4x4 matrix
    Matrix4x4 vp = m_camera->GetViewProjectionMatrix();
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            EXPECT_TRUE(std::isfinite(vp[c][r]));
        }
    }
}

// ============================================================================
// Camera — Movement (requires Scene context for Transform)
// ============================================================================
TEST_F(CameraWithSceneTest, MoveWorld) {
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);
    transform->SetPosition(Vector3(0.0f, 0.0f, 0.0f));

    m_camera->MoveWorld(10.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(transform->GetPosition().x, 10.0f);
    EXPECT_FLOAT_EQ(transform->GetPosition().y, 0.0f);
    EXPECT_FLOAT_EQ(transform->GetPosition().z, 0.0f);

    m_camera->MoveWorld(Vector3(0.0f, 5.0f, 0.0f));
    EXPECT_FLOAT_EQ(transform->GetPosition().x, 10.0f);
    EXPECT_FLOAT_EQ(transform->GetPosition().y, 5.0f);
    EXPECT_FLOAT_EQ(transform->GetPosition().z, 0.0f);
}

TEST_F(CameraWithSceneTest, MoveLocal) {
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);
    transform->SetPosition(Vector3(0.0f, 0.0f, 0.0f));

    // Move forward (default forward is +Z)
    m_camera->MoveLocal(5.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(transform->GetPosition().z, 5.0f);

    // Move right
    m_camera->MoveLocal(0.0f, 3.0f, 0.0f);
    EXPECT_FLOAT_EQ(transform->GetPosition().x, 3.0f);

    // Move up
    m_camera->MoveLocal(0.0f, 0.0f, 2.0f);
    EXPECT_FLOAT_EQ(transform->GetPosition().y, 2.0f);
}

TEST_F(CameraWithSceneTest, RotateDegrees) {
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);

    // Rotate 90° yaw (around Y) in degrees
    m_camera->Rotate(0.0f, 90.0f, 0.0f);

    // Forward should now be +X (rotated 90° from +Z)
    Vector3 fwd = m_camera->GetForward();
    EXPECT_NEAR(fwd.x, 1.0f, 1e-5f);
    EXPECT_NEAR(fwd.y, 0.0f, 1e-5f);
    EXPECT_NEAR(fwd.z, 0.0f, 1e-5f);
}

TEST_F(CameraWithSceneTest, MoveLocalAfterRotation) {
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);
    transform->SetPosition(Vector3(0.0f, 0.0f, 0.0f));

    // Rotate 90° yaw → forward = +X
    m_camera->Rotate(0.0f, 90.0f, 0.0f);

    // Move forward in local space → should move along +X
    m_camera->MoveLocal(5.0f, 0.0f, 0.0f);
    EXPECT_NEAR(transform->GetPosition().x, 5.0f, 1e-5f);
    EXPECT_NEAR(transform->GetPosition().y, 0.0f, 1e-5f);
    EXPECT_NEAR(transform->GetPosition().z, 0.0f, 1e-5f);
}

// ============================================================================
// Camera — Data Serialization
// ============================================================================
TEST(CameraTest, DataRoundTrip) {
    Graphic::Camera cam;

    Graphic::Camera::Data d;
    d.projectionMode = Graphic::ProjectionMode::Orthographic;
    d.fovDeg = 90.0f;
    d.orthoSize = 20.0f;
    d.nearPlane = 1.0f;
    d.farPlane = 500.0f;
    d.clearColor = {0.1f, 0.2f, 0.3f, 0.4f};

    cam.SetData(d);
    Graphic::Camera::Data out = cam.GetData();

    EXPECT_EQ(out.projectionMode, Graphic::ProjectionMode::Orthographic);
    EXPECT_FLOAT_EQ(out.fovDeg, 90.0f);
    EXPECT_FLOAT_EQ(out.orthoSize, 20.0f);
    EXPECT_FLOAT_EQ(out.nearPlane, 1.0f);
    EXPECT_FLOAT_EQ(out.farPlane, 500.0f);
    EXPECT_FLOAT_EQ(out.clearColor[0], 0.1f);
    EXPECT_FLOAT_EQ(out.clearColor[1], 0.2f);
    EXPECT_FLOAT_EQ(out.clearColor[2], 0.3f);
    EXPECT_FLOAT_EQ(out.clearColor[3], 0.4f);
}

// ============================================================================
// Camera — Get position/forward/up/right without a Transform
// (Camera without owner node falls back to zeros)
// ============================================================================
TEST(CameraTest, DefaultPositionWithoutTransform) {
    Graphic::Camera cam;
    Vector3 pos = cam.GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
    EXPECT_FLOAT_EQ(pos.z, 0.0f);
}

// ============================================================================
// Camera — Position with Transform
// ============================================================================
TEST_F(CameraWithSceneTest, GetPositionFromTransform) {
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);
    transform->SetPosition(Vector3(15.0f, 25.0f, 35.0f));

    Vector3 pos = m_camera->GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 15.0f);
    EXPECT_FLOAT_EQ(pos.y, 25.0f);
    EXPECT_FLOAT_EQ(pos.z, 35.0f);
}

// ============================================================================
// Camera — View matrix is valid after creation
// ============================================================================
TEST_F(CameraWithSceneTest, ViewMatrixIsValidAfterCreation) {
    auto transform = m_camera->GetTransform();
    ASSERT_NE(transform, nullptr);
    transform->SetPosition(Vector3(0.0f, 0.0f, 10.0f));

    Matrix4x4 view = m_camera->GetViewMatrix();

    // View matrix should not be identity when camera is positioned away from origin
    EXPECT_NE(view, Matrix4x4(1.0f));

    // Bottom row should be [0,0,0,1] for an affine transform
    EXPECT_FLOAT_EQ(view[0][3], 0.0f);
    EXPECT_FLOAT_EQ(view[1][3], 0.0f);
    EXPECT_FLOAT_EQ(view[2][3], 0.0f);
    EXPECT_FLOAT_EQ(view[3][3], 1.0f);

    // All elements should be finite
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            EXPECT_TRUE(std::isfinite(view[c][r]));
        }
    }
}

} // namespace
} // namespace Prisma
