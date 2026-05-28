#include <gtest/gtest.h>
#include "transform/Transform.h"

namespace Prisma {
namespace {

// ============================================================================
// Transform — Default State
// ============================================================================
TEST(TransformTest, DefaultValues) {
    Transform t;

    // Position: (0, 0, 0)
    EXPECT_FLOAT_EQ(t.GetPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(t.GetPosition().y, 0.0f);
    EXPECT_FLOAT_EQ(t.GetPosition().z, 0.0f);

    // Rotation: identity quaternion (x, y, z, w)
    EXPECT_FLOAT_EQ(t.GetRotation().x, 0.0f);
    EXPECT_FLOAT_EQ(t.GetRotation().y, 0.0f);
    EXPECT_FLOAT_EQ(t.GetRotation().z, 0.0f);
    EXPECT_FLOAT_EQ(t.GetRotation().w, 1.0f);

    // Scale: (1, 1, 1)
    EXPECT_FLOAT_EQ(t.GetScale().x, 1.0f);
    EXPECT_FLOAT_EQ(t.GetScale().y, 1.0f);
    EXPECT_FLOAT_EQ(t.GetScale().z, 1.0f);

    // Matrix: identity (first call triggers dirty → recompute)
    const Matrix4x4& mat = t.GetMatrix();
    EXPECT_FLOAT_EQ(mat[0][0], 1.0f); EXPECT_FLOAT_EQ(mat[0][1], 0.0f); EXPECT_FLOAT_EQ(mat[0][2], 0.0f); EXPECT_FLOAT_EQ(mat[0][3], 0.0f);
    EXPECT_FLOAT_EQ(mat[1][0], 0.0f); EXPECT_FLOAT_EQ(mat[1][1], 1.0f); EXPECT_FLOAT_EQ(mat[1][2], 0.0f); EXPECT_FLOAT_EQ(mat[1][3], 0.0f);
    EXPECT_FLOAT_EQ(mat[2][0], 0.0f); EXPECT_FLOAT_EQ(mat[2][1], 0.0f); EXPECT_FLOAT_EQ(mat[2][2], 1.0f); EXPECT_FLOAT_EQ(mat[2][3], 0.0f);
    EXPECT_FLOAT_EQ(mat[3][0], 0.0f); EXPECT_FLOAT_EQ(mat[3][1], 0.0f); EXPECT_FLOAT_EQ(mat[3][2], 0.0f); EXPECT_FLOAT_EQ(mat[3][3], 1.0f);

    // Component type ID
    EXPECT_GT(t.GetComponentId(), 0u);
    EXPECT_STREQ(t.GetComponentTypeName(), "Transform");
}

// ============================================================================
// Transform — Set Position (translation matrix)
// ============================================================================
TEST(TransformTest, SetPositionAndGetMatrix) {
    Transform t;
    t.SetPosition(Vector3(10.0f, 20.0f, 30.0f));

    const Matrix4x4& mat = t.GetMatrix();

    // Translation is in last column (column-major: mat[3])
    EXPECT_FLOAT_EQ(mat[3][0], 10.0f);
    EXPECT_FLOAT_EQ(mat[3][1], 20.0f);
    EXPECT_FLOAT_EQ(mat[3][2], 30.0f);
    EXPECT_FLOAT_EQ(mat[3][3], 1.0f);

    // Rotation/scale part remains identity
    EXPECT_FLOAT_EQ(mat[0][0], 1.0f);
    EXPECT_FLOAT_EQ(mat[1][1], 1.0f);
    EXPECT_FLOAT_EQ(mat[2][2], 1.0f);
}

// ============================================================================
// Transform — Set Rotation via Quaternion
// ============================================================================
TEST(TransformTest, SetRotationQuaternion) {
    Transform t;

    // Rotate 90° around Y axis
    Quaternion rot = glm::angleAxis(glm::radians(90.0f), Vector3(0.0f, 1.0f, 0.0f));
    t.SetRotation(rot);

    const Matrix4x4& mat = t.GetMatrix();

    // Use GLM to compute the expected rotation matrix
    Matrix4x4 expectedR = glm::mat4_cast(rot);

    EXPECT_NEAR(mat[0][0], expectedR[0][0], 1e-5f);
    EXPECT_NEAR(mat[0][1], expectedR[0][1], 1e-5f);
    EXPECT_NEAR(mat[0][2], expectedR[0][2], 1e-5f);
    EXPECT_NEAR(mat[1][0], expectedR[1][0], 1e-5f);
    EXPECT_NEAR(mat[1][1], expectedR[1][1], 1e-5f);
    EXPECT_NEAR(mat[1][2], expectedR[1][2], 1e-5f);
    EXPECT_NEAR(mat[2][0], expectedR[2][0], 1e-5f);
    EXPECT_NEAR(mat[2][1], expectedR[2][1], 1e-5f);
    EXPECT_NEAR(mat[2][2], expectedR[2][2], 1e-5f);

    // Verify forward direction is rotated correctly
    Vector3 fwd = t.GetForward();
    EXPECT_NEAR(fwd.x, 1.0f, 1e-5f);
    EXPECT_NEAR(fwd.y, 0.0f, 1e-5f);
    EXPECT_NEAR(fwd.z, 0.0f, 1e-5f);
}

TEST(TransformTest, SetRotationEuler) {
    Transform t;

    // Set via euler angles (degrees): 90° yaw
    t.SetRotation(Vector3(0.0f, 90.0f, 0.0f));

    // The result should match glm::quat(radians(euler))
    Quaternion expectedQ = Quaternion(glm::radians(Vector3(0.0f, 90.0f, 0.0f)));

    Matrix4x4 expectedR = glm::mat4_cast(expectedQ);
    const Matrix4x4& mat = t.GetMatrix();

    EXPECT_NEAR(mat[0][0], expectedR[0][0], 1e-5f);
    EXPECT_NEAR(mat[0][1], expectedR[0][1], 1e-5f);
    EXPECT_NEAR(mat[0][2], expectedR[0][2], 1e-5f);
    EXPECT_NEAR(mat[1][0], expectedR[1][0], 1e-5f);
    EXPECT_NEAR(mat[1][1], expectedR[1][1], 1e-5f);
    EXPECT_NEAR(mat[1][2], expectedR[1][2], 1e-5f);
    EXPECT_NEAR(mat[2][0], expectedR[2][0], 1e-5f);
    EXPECT_NEAR(mat[2][1], expectedR[2][1], 1e-5f);
    EXPECT_NEAR(mat[2][2], expectedR[2][2], 1e-5f);

    // Verify forward direction
    Vector3 fwd = t.GetForward();
    EXPECT_NEAR(fwd.x, 1.0f, 1e-5f);
    EXPECT_NEAR(fwd.y, 0.0f, 1e-5f);
    EXPECT_NEAR(fwd.z, 0.0f, 1e-5f);
}

// ============================================================================
// Transform — Set Scale
// ============================================================================
TEST(TransformTest, SetScale) {
    Transform t;
    t.SetScale(Vector3(2.0f, 3.0f, 4.0f));

    const Matrix4x4& mat = t.GetMatrix();

    EXPECT_FLOAT_EQ(mat[0][0], 2.0f);
    EXPECT_FLOAT_EQ(mat[1][1], 3.0f);
    EXPECT_FLOAT_EQ(mat[2][2], 4.0f);
    EXPECT_FLOAT_EQ(mat[3][3], 1.0f);  // homogeneous
}

// ============================================================================
// Transform — Translate + Rotate + Scale Combination
// ============================================================================
TEST(TransformTest, TranslateRotateScaleCombination) {
    Transform t;
    t.SetPosition(Vector3(5.0f, 10.0f, 15.0f));

    // 45° around Y
    Quaternion rot = glm::angleAxis(glm::radians(45.0f), Vector3(0.0f, 1.0f, 0.0f));
    t.SetRotation(rot);

    t.SetScale(Vector3(2.0f, 1.0f, 2.0f));

    const Matrix4x4& mat = t.GetMatrix();

    // Expected: T * R * S
    Matrix4x4 expected = glm::translate(Matrix4x4(1.0f), Vector3(5.0f, 10.0f, 15.0f))
                       * glm::mat4_cast(rot)
                       * glm::scale(Matrix4x4(1.0f), Vector3(2.0f, 1.0f, 2.0f));

    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            EXPECT_NEAR(mat[c][r], expected[c][r], 1e-5f);
        }
    }
}

// ============================================================================
// Transform — Dirty Flag Mechanism
// ============================================================================
TEST(TransformTest, DirtyFlagRecomputesMatrix) {
    Transform t;

    // First GetMatrix() recomputes from defaults → dirty cleared
    [[maybe_unused]] const Matrix4x4& initial = t.GetMatrix();

    // Modify position → dirty flag set
    t.SetPosition(Vector3(42.0f, 0.0f, 0.0f));

    // GetMatrix must recompute (translation should appear)
    const Matrix4x4& mat = t.GetMatrix();
    EXPECT_FLOAT_EQ(mat[3][0], 42.0f);

    // After GetMatrix, internal dirty flag should be clear.
    // A second call should return the same cached value.
    // (We verify by checking that no recomputation changes the result.)
    const Matrix4x4& mat2 = t.GetMatrix();
    EXPECT_FLOAT_EQ(mat2[3][0], 42.0f);

    // Now set a new position again — dirty flag triggered
    t.SetPosition(Vector3(99.0f, 0.0f, 0.0f));
    const Matrix4x4& mat3 = t.GetMatrix();
    EXPECT_FLOAT_EQ(mat3[3][0], 99.0f);
}

// ============================================================================
// Transform — GetForward Direction
// ============================================================================
TEST(TransformTest, GetForward) {
    Transform t;

    // Default forward should be +Z
    Vector3 fwd = t.GetForward();
    EXPECT_FLOAT_EQ(fwd.x, 0.0f);
    EXPECT_FLOAT_EQ(fwd.y, 0.0f);
    EXPECT_FLOAT_EQ(fwd.z, 1.0f);

    // Rotate 90° around Y → forward should be +X
    t.SetRotation(glm::angleAxis(glm::radians(90.0f), Vector3(0.0f, 1.0f, 0.0f)));
    fwd = t.GetForward();
    EXPECT_NEAR(fwd.x,  1.0f, 1e-5f);
    EXPECT_NEAR(fwd.y,  0.0f, 1e-5f);
    EXPECT_NEAR(fwd.z,  0.0f, 1e-5f);

    // Rotate -90° around Y → forward should be -X
    t.SetRotation(glm::angleAxis(glm::radians(-90.0f), Vector3(0.0f, 1.0f, 0.0f)));
    fwd = t.GetForward();
    EXPECT_NEAR(fwd.x, -1.0f, 1e-5f);
    EXPECT_NEAR(fwd.y,  0.0f, 1e-5f);
    EXPECT_NEAR(fwd.z,  0.0f, 1e-5f);
}

// ============================================================================
// Transform — GetYaw
// ============================================================================
TEST(TransformTest, GetYaw) {
    Transform t;

    // Default yaw (no rotation) should be 0
    EXPECT_NEAR(t.GetYaw(), 0.0f, 1e-5f);

    // Rotate 45° around Y → yaw = 45° in radians
    t.SetRotation(glm::angleAxis(glm::radians(45.0f), Vector3(0.0f, 1.0f, 0.0f)));
    EXPECT_NEAR(t.GetYaw(), glm::radians(45.0f), 1e-5f);

    // Rotate -90° around Y → yaw = -90° in radians
    t.SetRotation(glm::angleAxis(glm::radians(-90.0f), Vector3(0.0f, 1.0f, 0.0f)));
    EXPECT_NEAR(t.GetYaw(), glm::radians(-90.0f), 1e-5f);
}

// ============================================================================
// Transform — GetEulerAngles
// ============================================================================
TEST(TransformTest, GetEulerAngles) {
    Transform t;

    // Default euler: all zeros
    Vector3 euler = t.GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.0f, 1e-5f);
    EXPECT_NEAR(euler.y, 0.0f, 1e-5f);
    EXPECT_NEAR(euler.z, 0.0f, 1e-5f);

    // Pitch-only rotation should round-trip cleanly
    t.SetRotation(Vector3(30.0f, 0.0f, 0.0f));
    Vector3 readback = t.GetEulerAngles();
    EXPECT_NEAR(readback.x, glm::radians(30.0f), 1e-4f);

    // Verify returned values are finite (function doesn't produce NaN)
    t.SetRotation(Vector3(45.0f, 45.0f, 45.0f));
    readback = t.GetEulerAngles();
    EXPECT_TRUE(std::isfinite(readback.x));
    EXPECT_TRUE(std::isfinite(readback.y));
    EXPECT_TRUE(std::isfinite(readback.z));
}

// ============================================================================
// Transform — Data Round-Trip (GetData / SetData)
// ============================================================================
TEST(TransformTest, DataRoundTrip) {
    Transform t;

    Transform::Data d;
    d.position = {1.0f, 2.0f, 3.0f};
    d.rotation = {0.0f, 0.7071f, 0.0f, 0.7071f};
    d.scale    = {2.0f, 2.0f, 2.0f};

    t.SetData(d);
    Transform::Data out = t.GetData();

    EXPECT_FLOAT_EQ(out.position[0], 1.0f);
    EXPECT_FLOAT_EQ(out.position[1], 2.0f);
    EXPECT_FLOAT_EQ(out.position[2], 3.0f);

    EXPECT_FLOAT_EQ(out.rotation[0], 0.0f);
    EXPECT_FLOAT_EQ(out.rotation[1], 0.7071f);
    EXPECT_NEAR(out.rotation[2], 0.0f, 1e-4f);
    EXPECT_NEAR(out.rotation[3], 0.7071f, 1e-4f);

    EXPECT_FLOAT_EQ(out.scale[0], 2.0f);
    EXPECT_FLOAT_EQ(out.scale[1], 2.0f);
    EXPECT_FLOAT_EQ(out.scale[2], 2.0f);
}

// ============================================================================
// Transform — Multiple Changes
// ============================================================================
TEST(TransformTest, MultipleChangesPreserveCorrectness) {
    Transform t;

    for (int i = 0; i < 10; ++i) {
        float posVal = static_cast<float>(i * 10);
        float scaleVal = static_cast<float>(i + 1);

        t.SetPosition(Vector3(posVal, posVal * 2, posVal * 3));

        Quaternion rot = glm::angleAxis(glm::radians(static_cast<float>(i * 15)), Vector3(0.0f, 1.0f, 0.0f));
        t.SetRotation(rot);

        t.SetScale(Vector3(scaleVal, scaleVal, scaleVal));

        const Matrix4x4& mat = t.GetMatrix();

        EXPECT_FLOAT_EQ(mat[3][0], posVal);
        EXPECT_FLOAT_EQ(mat[3][1], posVal * 2.0f);
        EXPECT_FLOAT_EQ(mat[3][2], posVal * 3.0f);
    }
}

} // namespace
} // namespace Prisma
