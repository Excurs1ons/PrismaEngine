#pragma once

// 跨平台数学库抽象层
// 根据平台选择不同的数学库实现

#if defined(_WIN32) || defined(_WIN64)
    // Windows平台使用DirectXMath
    #include <DirectXMath.h>
    using namespace DirectX;

    // 类型别名，使代码更清晰
    using Vector2 = XMFLOAT2;
    using Vector3 = XMFLOAT3;
    using Vector4 = XMFLOAT4;
    using Matrix4x4 = XMFLOAT4X4;
    using Quaternion = XMFLOAT4;

#else
    // 非Windows平台使用GLM
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

#endif

// 统一的数学函数接口
namespace Prisma {
namespace Math {

#if defined(_WIN32) || defined(_WIN64)

// Windows平台实现（使用DirectXMath）
inline XMVECTOR ToVector(const Vector3& v) {
    return XMLoadFloat3(&v);
}

inline XMVECTOR ToVector(const Vector4& v) {
    return XMLoadFloat4(&v);
}

inline XMMATRIX ToMatrix(const Matrix4x4& m) {
    return XMLoadFloat4x4(&m);
}

inline Vector3 FromVector(const XMVECTOR& v) {
    Vector3 result;
    XMStoreFloat3(&result, v);
    return result;
}

inline Vector4 FromVector4(const XMVECTOR& v) {
    Vector4 result;
    XMStoreFloat4(&result, v);
    return result;
}

inline Matrix4x4 FromMatrix(const XMMATRIX& m) {
    Matrix4x4 result;
    XMStoreFloat4x4(&result, m);
    return result;
}

// 向量运算
inline Vector3 Add(const Vector3& a, const Vector3& b) {
    return FromVector(ToVector(a) + ToVector(b));
}

inline Vector3 Subtract(const Vector3& a, const Vector3& b) {
    return FromVector(ToVector(a) - ToVector(b));
}

inline Vector3 Multiply(const Vector3& v, float s) {
    return FromVector(ToVector(v) * s);
}

inline float Dot(const Vector3& a, const Vector3& b) {
    return XMVectorGetX(XMVector3Dot(ToVector(a), ToVector(b)));
}

inline Vector3 Cross(const Vector3& a, const Vector3& b) {
    return FromVector(XMVector3Cross(ToVector(a), ToVector(b)));
}

inline float Length(const Vector3& v) {
    return XMVectorGetX(XMVector3Length(ToVector(v)));
}

inline Vector3 Normalize(const Vector3& v) {
    return FromVector(XMVector3Normalize(ToVector(v)));
}

// 矩阵运算
inline Matrix4x4 Identity() {
    return FromMatrix(XMMatrixIdentity());
}

inline Matrix4x4 Translation(const Vector3& t) {
    return FromMatrix(XMMatrixTranslation(t.x, t.y, t.z));
}

inline Matrix4x4 RotationX(float angle) {
    return FromMatrix(XMMatrixRotationX(angle));
}

inline Matrix4x4 RotationY(float angle) {
    return FromMatrix(XMMatrixRotationY(angle));
}

inline Matrix4x4 RotationZ(float angle) {
    return FromMatrix(XMMatrixRotationZ(angle));
}

inline Matrix4x4 Scale(const Vector3& s) {
    return FromMatrix(XMMatrixScaling(s.x, s.y, s.z));
}

inline Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b) {
    return FromMatrix(ToMatrix(a) * ToMatrix(b));
}

inline Matrix4x4 Transpose(const Matrix4x4& m) {
    return FromMatrix(XMMatrixTranspose(ToMatrix(m)));
}

inline Matrix4x4 Inverse(const Matrix4x4& m) {
    XMVECTOR det;
    return FromMatrix(XMMatrixInverse(&det, ToMatrix(m)));
}

// 四元数运算
inline Quaternion QuaternionIdentity() {
    return FromVector4(XMQuaternionIdentity());
}

inline Quaternion CreateFromAxisAngle(const Vector3& axis, float angle) {
    return FromVector4(XMQuaternionRotationAxis(ToVector(axis), angle));
}

inline Quaternion Multiply(const Quaternion& a, const Quaternion& b) {
    return FromVector4(XMQuaternionMultiply(ToVector4(a), ToVector4(b)));
}

inline Matrix4x4 QuaternionToMatrix(const Quaternion& q) {
    return FromMatrix(XMMatrixRotationQuaternion(ToVector4(q)));
}

// 视角投影矩阵
inline Matrix4x4 PerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ) {
    return FromMatrix(XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ));
}

inline Matrix4x4 OrthographicLH(float viewWidth, float viewHeight, float nearZ, float farZ) {
    return FromMatrix(XMMatrixOrthographicLH(viewWidth, viewHeight, nearZ, farZ));
}

#else

// 非Windows平台实现（使用GLM）

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

// 视角投影矩阵
inline Matrix4x4 PerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ) {
    return glm::perspectiveLH(fovAngleY, aspectRatio, nearZ, farZ);
}

inline Matrix4x4 OrthographicLH(float viewWidth, float viewHeight, float nearZ, float farZ) {
    return glm::orthoLH(0.0f, viewWidth, 0.0f, viewHeight, nearZ, farZ);
}

#endif

// 通用常量
constexpr float PI = 3.14159265358979323846f;
constexpr float TwoPI = 2.0f * PI;
constexpr float HalfPI = PI * 0.5f;

} // namespace Math
} // namespace Prisma