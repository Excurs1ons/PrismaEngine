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

// 四元数向量化操作
inline Vector4 Normalize(const Vector4& q) {
    return FromVector4(XMQuaternionNormalize(ToVector4(q)));
}

inline float Length(const Vector4& q) {
    return XMVectorGetX(XMQuaternionLength(ToVector4(q)));
}

inline float LengthSquared(const Vector4& q) {
    return XMVectorGetX(XMQuaternionLengthSq(ToVector4(q)));
}

inline Vector4 Inverse(const Vector4& q) {
    return FromVector4(XMQuaternionInverse(ToVector4(q)));
}

inline float Dot(const Vector4& a, const Vector4& b) {
    return XMVectorGetX(XMVector4Dot(ToVector4(a), ToVector4(b)));
}

inline Vector4 Slerp(const Vector4& a, const Vector4& b, float t) {
    return FromVector4(XMQuaternionSlerp(ToVector4(a), ToVector4(b), t));
}

inline Vector4 FromEulerAngles(float pitch, float yaw, float roll) {
    return FromVector4(XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
}

inline Vector4 FromAxisAngle(const Vector3& axis, float angle) {
    return FromVector4(XMQuaternionRotationAxis(ToVector(axis), angle));
}

inline Vector4 FromRotationMatrix(const Matrix4x4& matrix) {
    return FromVector4(XMQuaternionRotationMatrix(ToMatrix(matrix)));
}

inline Vector3 ToEulerAngles(const Vector4& q) {
    // 提取欧拉角 (pitch, yaw, roll)
    XMVECTOR quat = ToVector4(q);
    float test = XMVectorGetX(quat) * XMVectorGetY(quat) + XMVectorGetZ(quat) * XMVectorGetW(quat);
    Vector3 euler;

    if (test > 0.499f) {
        // 奇点处理：north pole
        euler.y = 2.0f * atan2f(XMVectorGetX(quat), XMVectorGetW(quat));
        euler.x = XM_PIDIV2;
        euler.z = 0;
    } else if (test < -0.499f) {
        // 奇点处理：south pole
        euler.y = -2.0f * atan2f(XMVectorGetX(quat), XMVectorGetW(quat));
        euler.x = -XM_PIDIV2;
        euler.z = 0;
    } else {
        float sqx = XMVectorGetX(quat) * XMVectorGetX(quat);
        float sqy = XMVectorGetY(quat) * XMVectorGetY(quat);
        float sqz = XMVectorGetZ(quat) * XMVectorGetZ(quat);
        euler.y = atan2f(2.0f * XMVectorGetY(quat) * XMVectorGetW(quat) - 2.0f * XMVectorGetX(quat) * XMVectorGetZ(quat),
                         1.0f - 2.0f * (sqy + sqz));
        euler.x = asinf(2.0f * test);
        euler.z = atan2f(2.0f * XMVectorGetX(quat) * XMVectorGetW(quat) - 2.0f * XMVectorGetY(quat) * XMVectorGetZ(quat),
                         1.0f - 2.0f * (sqx + sqz));
    }

    return euler;
}

inline Vector4 LookRotation(const Vector3& forward, const Vector3& up) {
    XMVECTOR f = ToVector(forward);
    XMVECTOR u = ToVector(up);

    // 标准化向量
    f = XMVector3Normalize(f);
    u = XMVector3Normalize(u);

    // 计算右向量
    XMVECTOR r = XMVector3Cross(u, f);
    r = XMVector3Normalize(r);

    // 重新计算上向量
    u = XMVector3Cross(f, r);

    // 创建旋转矩阵
    XMMATRIX rotationMatrix = XMMATRIX(
        XMVectorGetX(r), XMVectorGetX(u), XMVectorGetX(f), 0.0f,
        XMVectorGetY(r), XMVectorGetY(u), XMVectorGetY(f), 0.0f,
        XMVectorGetZ(r), XMVectorGetZ(u), XMVectorGetZ(f), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    XMVECTOR rotationQuat = XMQuaternionRotationMatrix(rotationMatrix);
    return FromVector4(rotationQuat);
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
    return glm::inverse(q);
}

inline float Dot(const Vector4& a, const Vector4& b) {
    return glm::dot(a, b);
}

inline Vector4 Slerp(const Vector4& a, const Vector4& b, float t) {
    return glm::slerp(a, b, t);
}

inline Vector4 FromEulerAngles(float pitch, float yaw, float roll) {
    return glm::quat(glm::vec3(pitch, yaw, roll));
}

inline Vector4 FromAxisAngle(const Vector3& axis, float angle) {
    return glm::angleAxis(angle, glm::normalize(axis));
}

inline Vector4 FromRotationMatrix(const Matrix4x4& matrix) {
    return glm::quat_cast(matrix);
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

#endif

// 角度转换函数
inline float Radians(float degrees) {
#if defined(_WIN32) || defined(_WIN64)
    return XMConvertToRadians(degrees);
#else
    return glm::radians(degrees);
#endif
}

inline float Degrees(float radians) {
#if defined(_WIN32) || defined(_WIN64)
    return XMConvertToDegrees(radians);
#else
    return glm::degrees(radians);
#endif
}

// 通用常量
constexpr float PI = 3.14159265358979323846f;
constexpr float TwoPI = 2.0f * PI;
constexpr float HalfPI = PI * 0.5f;

} // namespace Math
} // namespace Prisma