#include <gtest/gtest.h>
#include "math/MathTypes.h"
#include "math/MatrixUtils.h"

using namespace Prisma;
using namespace Prisma::Math;

// ---- Plane ----

TEST(MathTypes, PlaneNormalize) {
    Plane p1;
    float len1 = glm::length(p1.normal);
    EXPECT_NEAR(len1, 1.0f, EPSILON);
    EXPECT_NEAR(p1.normal.x, 0.0f, EPSILON);
    EXPECT_NEAR(p1.normal.y, 1.0f, EPSILON);
    EXPECT_NEAR(p1.normal.z, 0.0f, EPSILON);
    EXPECT_NEAR(p1.distance, 0.0f, EPSILON);

    Plane p2(0.0f, 1.0f, 0.0f, -5.0f);
    float len2 = glm::length(p2.normal);
    EXPECT_NEAR(len2, 1.0f, EPSILON);
    EXPECT_NEAR(p2.distance, -5.0f, EPSILON);

    Vector3 n = glm::normalize(Vector3(1.0f, 2.0f, 3.0f));
    Plane p3(n, 10.0f);
    float len3 = glm::length(p3.normal);
    EXPECT_NEAR(len3, 1.0f, EPSILON);
    EXPECT_NEAR(p3.distance, 10.0f, EPSILON);
}

TEST(MathTypes, PlaneDistance) {
    Plane p(0.0f, 1.0f, 0.0f, -5.0f);

    float dOn = glm::dot(p.normal, Vector3(0.0f, 5.0f, 0.0f)) + p.distance;
    EXPECT_NEAR(dOn, 0.0f, EPSILON);

    float dAbove = glm::dot(p.normal, Vector3(0.0f, 10.0f, 0.0f)) + p.distance;
    EXPECT_NEAR(dAbove, 5.0f, EPSILON);

    float dBelow = glm::dot(p.normal, Vector3(0.0f, 0.0f, 0.0f)) + p.distance;
    EXPECT_NEAR(dBelow, -5.0f, EPSILON);
}

TEST(MathTypes, PlaneIntersects) {
    Plane p(0.0f, 1.0f, 0.0f, -3.0f);

    Vector3 front(0.0f, 5.0f, 0.0f);
    float side1 = glm::dot(p.normal, front) + p.distance;
    EXPECT_GT(side1, 0.0f);

    Vector3 on(0.0f, 3.0f, 0.0f);
    float side2 = glm::dot(p.normal, on) + p.distance;
    EXPECT_NEAR(side2, 0.0f, EPSILON);

    Vector3 back(0.0f, 1.0f, 0.0f);
    float side3 = glm::dot(p.normal, back) + p.distance;
    EXPECT_LT(side3, 0.0f);
}

// ---- Type aliases ----

TEST(MathTypes, Vec3Type) {
    EXPECT_EQ(sizeof(Vector3), sizeof(float) * 3);
    EXPECT_EQ(sizeof(IVector3), sizeof(int32_t) * 3);
    EXPECT_EQ(sizeof(UVector3), sizeof(uint32_t) * 3);
    EXPECT_EQ(sizeof(Vector2), sizeof(float) * 2);
    EXPECT_EQ(sizeof(Matrix4x4), sizeof(float) * 16);
    EXPECT_EQ(sizeof(Matrix3x3), sizeof(float) * 9);
    EXPECT_EQ(sizeof(Quaternion), sizeof(float) * 4);
}

// ---- MatrixUtils / Math helpers ----

TEST(MatrixUtils, CreateTranslation) {
    Matrix4x4 m = Math::Translation(Vector3(1.0f, 2.0f, 3.0f));
    EXPECT_NEAR(m[3][0], 1.0f, EPSILON);
    EXPECT_NEAR(m[3][1], 2.0f, EPSILON);
    EXPECT_NEAR(m[3][2], 3.0f, EPSILON);
    EXPECT_NEAR(m[3][3], 1.0f, EPSILON);
    EXPECT_NEAR(m[0][0], 1.0f, EPSILON);
    EXPECT_NEAR(m[1][1], 1.0f, EPSILON);
    EXPECT_NEAR(m[2][2], 1.0f, EPSILON);

    Matrix4x4 identity = Math::Translation(Vector3(0.0f));
    EXPECT_NEAR(identity[3][0], 0.0f, EPSILON);
    EXPECT_NEAR(identity[3][1], 0.0f, EPSILON);
    EXPECT_NEAR(identity[3][2], 0.0f, EPSILON);
}

TEST(MatrixUtils, CreateRotation) {
    // Note: GLM uses column-major storage (m[col][row]).
    // For R_x(pi/2): columns are (1,0,0), (0,0,1), (0,-1,0)
    Matrix4x4 rx = Math::RotationX(HALF_PI);
    EXPECT_NEAR(rx[0][0], 1.0f, EPSILON);
    EXPECT_NEAR(rx[1][1], 0.0f, EPSILON);
    EXPECT_NEAR(rx[1][2], 1.0f, EPSILON);
    EXPECT_NEAR(rx[2][1], -1.0f, EPSILON);
    EXPECT_NEAR(rx[2][2], 0.0f, EPSILON);

    // For R_y(pi/2): columns are (0,0,-1), (0,1,0), (1,0,0)
    Matrix4x4 ry = Math::RotationY(HALF_PI);
    EXPECT_NEAR(ry[0][0], 0.0f, EPSILON);
    EXPECT_NEAR(ry[0][2], -1.0f, EPSILON);
    EXPECT_NEAR(ry[2][0], 1.0f, EPSILON);
    EXPECT_NEAR(ry[2][2], 0.0f, EPSILON);

    // For R_z(pi/2): columns are (0,1,0), (-1,0,0), (0,0,1)
    Matrix4x4 rz = Math::RotationZ(HALF_PI);
    EXPECT_NEAR(rz[0][0], 0.0f, EPSILON);
    EXPECT_NEAR(rz[0][1], 1.0f, EPSILON);
    EXPECT_NEAR(rz[1][0], -1.0f, EPSILON);
    EXPECT_NEAR(rz[1][1], 0.0f, EPSILON);

    Matrix4x4 r0 = Math::RotationX(0.0f);
    EXPECT_NEAR(r0[0][0], 1.0f, EPSILON);
    EXPECT_NEAR(r0[1][1], 1.0f, EPSILON);
    EXPECT_NEAR(r0[2][2], 1.0f, EPSILON);
}

TEST(MatrixUtils, CreateScale) {
    Matrix4x4 m = Math::Scale(Vector3(2.0f, 3.0f, 4.0f));
    EXPECT_NEAR(m[0][0], 2.0f, EPSILON);
    EXPECT_NEAR(m[1][1], 3.0f, EPSILON);
    EXPECT_NEAR(m[2][2], 4.0f, EPSILON);

    Matrix4x4 identity = Math::Scale(Vector3(1.0f));
    EXPECT_NEAR(identity[0][0], 1.0f, EPSILON);
    EXPECT_NEAR(identity[1][1], 1.0f, EPSILON);
    EXPECT_NEAR(identity[2][2], 1.0f, EPSILON);

    Matrix4x4 mirror = Math::Scale(Vector3(-1.0f, 1.0f, 1.0f));
    EXPECT_NEAR(mirror[0][0], -1.0f, EPSILON);
    EXPECT_NEAR(mirror[1][1], 1.0f, EPSILON);
    EXPECT_NEAR(mirror[2][2], 1.0f, EPSILON);
}

TEST(MatrixUtils, CreateLookAt) {
    Vector3 eye(0.0f, 0.0f, 5.0f);
    Vector3 target(0.0f, 0.0f, 0.0f);
    Vector3 up(0.0f, 1.0f, 0.0f);

    Matrix4x4 view = MatrixUtils::CreateLookAt(eye, target, up);
    EXPECT_GT(std::abs(glm::determinant(view)), 0.001f);
    EXPECT_NEAR(view[3][3], 1.0f, EPSILON);

    Matrix4x4 view2 = Math::LookAt(eye, target, up);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            EXPECT_NEAR(view[c][r], view2[c][r], EPSILON);
}

TEST(MatrixUtils, CreatePerspective) {
    float fov = glm::radians(60.0f);
    float aspect = 16.0f / 9.0f;
    float nearP = 0.1f, farP = 100.0f;

    Matrix4x4 proj = MatrixUtils::CreatePerspective(fov, aspect, nearP, farP);
    EXPECT_GT(proj[0][0], 0.0f);
    EXPECT_GT(proj[1][1], 0.0f);
    EXPECT_LT(proj[2][2], 0.0f);
    EXPECT_NEAR(proj[3][2], -2.0f * farP * nearP / (farP - nearP), EPSILON);
    EXPECT_GT(std::abs(glm::determinant(proj)), 0.001f);

    Matrix4x4 proj2 = Math::Perspective(fov, aspect, nearP, farP);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            EXPECT_NEAR(proj[c][r], proj2[c][r], EPSILON);
}

TEST(MatrixUtils, CreateOrthographic) {
    float left = -10.0f, right = 10.0f;
    float bottom = -5.0f, top = 5.0f;
    float nearP = 0.0f, farP = 100.0f;

    Matrix4x4 ortho = MatrixUtils::CreateOrthographic(left, right, bottom, top, nearP, farP);
    EXPECT_GT(ortho[0][0], 0.0f);
    EXPECT_GT(ortho[1][1], 0.0f);
    EXPECT_LT(ortho[2][2], 0.0f);
    EXPECT_NEAR(ortho[3][3], 1.0f, EPSILON);
    EXPECT_GT(std::abs(glm::determinant(ortho)), 0.0001f);

    Matrix4x4 ortho2 = Math::Orthographic(left, right, bottom, top, nearP, farP);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            EXPECT_NEAR(ortho[c][r], ortho2[c][r], EPSILON);
}
