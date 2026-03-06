#pragma once

// ============================================================================
// PrismaEngine Math Types
// 统一使用 GLM (OpenGL Mathematics) 作为底层数学库
// ============================================================================

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <numbers>

namespace PrismaEngine {

// 向量类型
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using IVector2 = glm::ivec2;
using IVector3 = glm::ivec3;
using IVector4 = glm::ivec4;
using UVector2 = glm::uvec2;
using UVector3 = glm::uvec3;
using UVector4 = glm::uvec4;

// 矩阵类型
using Matrix3x3 = glm::mat3;
using Matrix3 = Matrix3x3;
using Matrix4x4 = glm::mat4;
using Matrix4 = Matrix4x4;

// 四元数类型
using Quaternion = glm::quat;

// 颜色类型
using Color = Vector4;

// 平面类型
struct Plane {
    Vector3 normal;
    float distance;

    Plane() : normal(0.0f, 1.0f, 0.0f), distance(0.0f) {}
    Plane(const Vector3& n, float d) : normal(n), distance(d) {}
    Plane(float a, float b, float c, float d) : normal(a, b, c), distance(d) {}
};

// 常用数学常量
constexpr float PI = std::numbers::pi_v<float>;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;
constexpr float QUARTER_PI = PI * 0.25f;
constexpr float INV_PI = 1.0f / PI;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

constexpr float EPSILON = 1e-6f;

// 角度/弧度转换
constexpr float Deg2Rad(float degrees) { return degrees * DEG_TO_RAD; }
constexpr float Rad2Deg(float radians) { return radians * RAD_TO_DEG; }

namespace Math {

// 仅保留特定领域的辅助函数，丢弃对 GLM 原生操作符 (+, -, *) 的冗余包装

inline Matrix4x4 Translation(const Vector3& t) {
    return glm::translate(glm::mat4(1.0f), t);
}

inline Matrix4x4 RotationX(float angle) {
    return glm::rotate(glm::mat4(1.0f), angle, Vector3(1.0f, 0.0f, 0.0f));
}

inline Matrix4x4 RotationY(float angle) {
    return glm::rotate(glm::mat4(1.0f), angle, Vector3(0.0f, 1.0f, 0.0f));
}

inline Matrix4x4 RotationZ(float angle) {
    return glm::rotate(glm::mat4(1.0f), angle, Vector3(0.0f, 0.0f, 1.0f));
}

inline Matrix4x4 Scale(const Vector3& s) {
    return glm::scale(glm::mat4(1.0f), s);
}

// 视角投影矩阵 (使用右手法则)
inline Matrix4x4 Perspective(float fov, float aspect, float nearPlane, float farPlane) {
    return glm::perspectiveRH(fov, aspect, nearPlane, farPlane);
}

inline Matrix4x4 Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    return glm::orthoRH(left, right, bottom, top, nearPlane, farPlane);
}

inline Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
    return glm::lookAtRH(eye, center, up);
}

inline Quaternion FromEulerAngles(const Vector3& euler) {
    return glm::quat(euler);
}

inline Vector3 ToEulerAngles(const Quaternion& q) {
    return glm::eulerAngles(q);
}

// 基于 Forward 和 Up 向量计算旋转四元数
inline Quaternion LookRotation(const Vector3& forward, const Vector3& up) {
    Vector3 f = glm::normalize(forward);
    Vector3 u = glm::normalize(up);
    Vector3 r = glm::normalize(glm::cross(u, f));
    u = glm::cross(f, r);

    Matrix4x4 rotationMatrix(
        r.x, r.y, r.z, 0.0f,
        u.x, u.y, u.z, 0.0f,
        -f.x, -f.y, -f.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    return glm::quat_cast(rotationMatrix);
}

} // namespace Math
} // namespace PrismaEngine
