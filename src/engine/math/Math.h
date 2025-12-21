#pragma once

// 跨平台数学库抽象层
// 统一使用GLM

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// 类型别名，统一接口
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Matrix4x4 = glm::mat4;
using Quaternion = glm::quat;

// 统一的数学函数接口
namespace Prisma::Math {

// 向量运算
inline Vector3 Add(const Vector3& a, const Vector3& b) {
    return a + b;
}

inline Vector3 Subtract(const Vector3& a, const Vector3& b) {
    return a - b;
}

inline Vector3 Multiply(const Vector3& v, float s) {
    return v * s;
}

inline Vector4 Multiply(const Vector4& v, float s) {
    return v * s;
}

inline Vector4 Multiply(const Vector4& v, const Vector4& s) {
    return {v.x*s.x, v.y*s.y, v.z*s.z, v.w*s.w};
}

inline float Dot(const Vector3& a, const Vector3& b) {
    return glm::dot(a, b);
}

inline Vector3 Cross(const Vector3& a, const Vector3& b) {
    return glm::cross(a, b);
}

inline float Length(const Vector3& v) {
    return glm::length(v);
}

inline Vector3 Normalize(const Vector3& v) {
    return glm::normalize(v);
}

// 矩阵运算
inline Matrix4x4 Identity() {
    return glm::mat4(1.0f);
}

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

inline Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b) {
    return a * b;
}

inline Matrix4x4 Transpose(const Matrix4x4& m) {
    return glm::transpose(m);
}

inline Matrix4x4 Inverse(const Matrix4x4& m) {
    return glm::inverse(m);
}

// 四元数运算
inline Quaternion QuaternionIdentity() {
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

inline Quaternion CreateFromAxisAngle(const Vector3& axis, float angle) {
    return glm::angleAxis(angle, glm::normalize(axis));
}

inline Quaternion Multiply(const Quaternion& a, const Quaternion& b) {
    return a * b;
}

inline Matrix4x4 QuaternionToMatrix(const Quaternion& q) {
    return glm::toMat4(q);
}

// 四元数向量化操作
inline Vector4 Normalize(const Vector4& q) {
    return glm::normalize(q);
}

inline float Length(const Vector4& q) {
    return glm::length(q);
}

inline float LengthSquared(const Vector4& q) {
    return glm::dot(q, q);
}

inline Vector4 Inverse(const Vector4& q) {
    // 对于四元数 Vector4，转换为 quat 后求逆
    glm::quat quat_q(q);
    quat_q = glm::inverse(quat_q);
    return Vector4(quat_q.x, quat_q.y, quat_q.z, quat_q.w);
}

inline float Dot(const Vector4& a, const Vector4& b) {
    return glm::dot(a, b);
}

inline Vector4 Slerp(const Vector4& a, const Vector4& b, float t) {
    // 对于四元数 Vector4，转换为 quat 后进行球面线性插值
    glm::quat quat_a(a);
    glm::quat quat_b(b);
    glm::quat result = glm::slerp(quat_a, quat_b, t);
    return Vector4(result.x, result.y, result.z, result.w);
}

inline Vector4 FromEulerAngles(float pitch, float yaw, float roll) {
    glm::quat result = glm::quat(glm::vec3(pitch, yaw, roll));
    return Vector4(result.x, result.y, result.z, result.w);
}

inline Vector4 FromAxisAngle(const Vector3& axis, float angle) {
    glm::quat result = glm::angleAxis(angle, glm::normalize(axis));
    return Vector4(result.x, result.y, result.z, result.w);
}

inline Vector4 FromRotationMatrix(const Matrix4x4& matrix) {
    glm::quat result = glm::quat_cast(matrix);
    return Vector4(result.x, result.y, result.z, result.w);
}

inline Vector3 ToEulerAngles(const Vector4& q) {
    return glm::eulerAngles(glm::quat(q.w, q.x, q.y, q.z));
}

inline Vector4 LookRotation(const Vector3& forward, const Vector3& up) {
    Vector3 f = glm::normalize(forward);
    Vector3 u = glm::normalize(up);

    // 计算右向量
    Vector3 r = glm::cross(u, f);
    r = glm::normalize(r);

    // 重新计算上向量
    u = glm::cross(f, r);

    // 创建旋转矩阵
    Matrix4x4 rotationMatrix = Matrix4x4(
        r.x, u.x, f.x, 0.0f,
        r.y, u.y, f.y, 0.0f,
        r.z, u.z, f.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    return glm::quat_cast(rotationMatrix);
}

// 视角投影矩阵
inline Matrix4x4 PerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ) {
    return glm::perspectiveLH(fovAngleY, aspectRatio, nearZ, farZ);
}

inline Matrix4x4 OrthographicLH(float viewWidth, float viewHeight, float nearZ, float farZ) {
    return glm::orthoLH(0.0f, viewWidth, 0.0f, viewHeight, nearZ, farZ);
}

inline Matrix4x4 Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ) {
    return glm::ortho(left, right, bottom, top, nearZ, farZ);
}

// 添加一些常用的函数
inline Matrix4x4 Perspective(float fov, float aspect, float nearPlane, float farPlane) {
    return glm::perspective(fov, aspect, nearPlane, farPlane);
}

inline Matrix4x4 lookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
    return glm::lookAt(eye, center, up);
}

inline Quaternion FromEulerAngles(const Vector3& euler) {
    return glm::quat(euler);
}

inline Vector4 ToQuaternion(const Quaternion& q) {
    return Vector4(q.x, q.y, q.z, q.w);
}

// 角度转换函数
inline float Radians(float degrees) {
    return glm::radians(degrees);
}

inline float Degrees(float radians) {
    return glm::degrees(radians);
}

// Clamp函数
inline float Clamp(float value, float min, float max) {
    return glm::clamp(value, min, max);
}

// Lerp插值函数
inline Vector4 Lerp(const Vector4& a, const Vector4& b, float t) {
    return glm::mix(a, b, t);
}

// 通用常量
constexpr float PI = 3.14159265358979323846f;
constexpr float TwoPI = 2.0f * PI;
constexpr float HalfPI = PI * 0.5f;

} // namespace Prisma::Math