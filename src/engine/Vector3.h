//
// Created by JasonGu on 25-12-14.
//

#ifndef VECTOR3_H
#define VECTOR3_H

#include "math/MathTypes.h"

struct Vector3 {
    // 跨平台数据存储
#if defined(PRISMA_USE_DIRECTXMATH)
    DirectX::XMFLOAT3 m_data;
#else
    glm::vec3 m_data;
#endif

    // 构造函数
    Vector3() {
#if defined(PRISMA_USE_DIRECTXMATH)
        m_data = DirectX::XMFLOAT3(0, 0, 0);
#else
        m_data = glm::vec3(0, 0, 0);
#endif
    }

    Vector3(float x, float y, float z) {
#if defined(PRISMA_USE_DIRECTXMATH)
        m_data = DirectX::XMFLOAT3(x, y, z);
#else
        m_data = glm::vec3(x, y, z);
#endif
    }

#if defined(PRISMA_USE_DIRECTXMATH)
    Vector3(const DirectX::XMFLOAT3& vector) : m_data(vector) {}
    Vector3(const DirectX::XMVECTOR& vector) {
        DirectX::XMStoreFloat3(&m_data, vector);
    }
#else
    Vector3(const glm::vec3& vector) : m_data(vector) {}
#endif

    // 便捷访问器
    float& x() { return m_data.x; }
    float& y() { return m_data.y; }
    float& z() { return m_data.z; }
    const float& x() const { return m_data.x; }
    const float& y() const { return m_data.y; }
    const float& z() const { return m_data.z; }

    // 为了向后兼容，保留直接成员访问
#if defined(PRISMA_USE_DIRECTXMATH)
    float& x = m_data.x;
    float& y = m_data.y;
    float& z = m_data.z;
    const float& x = m_data.x;
    const float& y = m_data.y;
    const float& z = m_data.z;
#endif

    // 转换操作符
#if defined(PRISMA_USE_DIRECTXMATH)
    operator DirectX::XMFLOAT3() const { return m_data; }
    operator DirectX::XMVECTOR() const {
        return DirectX::XMLoadFloat3(&m_data);
    }
#else
    operator glm::vec3() const { return m_data; }
#endif

    // 赋值操作符
    Vector3& operator=(const Vector3& other) {
        m_data = other.m_data;
        return *this;
    }

#if defined(PRISMA_USE_DIRECTXMATH)
    Vector3& operator=(const DirectX::XMFLOAT3& vector) {
        m_data = vector;
        return *this;
    }
    Vector3& operator=(const DirectX::XMVECTOR& vector) {
        DirectX::XMStoreFloat3(&m_data, vector);
        return *this;
    }
#else
    Vector3& operator=(const glm::vec3& vector) {
        m_data = vector;
        return *this;
    }
#endif

    // 数学运算符
    Vector3 operator+(const Vector3& other) const {
#if defined(PRISMA_USE_DIRECTXMATH)
        return DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&m_data), DirectX::XMLoadFloat3(&other.m_data));
#else
        return Vector3(m_data + other.m_data);
#endif
    }

    Vector3 operator-(const Vector3& other) const {
#if defined(PRISMA_USE_DIRECTXMATH)
        return DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&m_data), DirectX::XMLoadFloat3(&other.m_data));
#else
        return Vector3(m_data - other.m_data);
#endif
    }

    Vector3 operator*(float scalar) const {
#if defined(PRISMA_USE_DIRECTXMATH)
        return DirectX::XMVectorScale(DirectX::XMLoadFloat3(&m_data), scalar);
#else
        return Vector3(m_data * scalar);
#endif
    }

    Vector3 operator/(float scalar) const {
        return *this * (1.0f / scalar);
    }

    Vector3 operator-() const {
        return *this * -1.0f;
    }

    // 复合赋值运算符
    Vector3& operator+=(const Vector3& other) {
        *this = *this + other;
        return *this;
    }

    Vector3& operator-=(const Vector3& other) {
        *this = *this - other;
        return *this;
    }

    Vector3& operator*=(float scalar) {
        *this = *this * scalar;
        return *this;
    }

    Vector3& operator/=(float scalar) {
        *this = *this / scalar;
        return *this;
    }

    // 比较运算符
    bool operator==(const Vector3& other) const {
#if defined(PRISMA_USE_DIRECTXMATH)
        return DirectX::XMVector3Equal(DirectX::XMLoadFloat3(&m_data), DirectX::XMLoadFloat3(&other.m_data));
#else
        return glm::all(glm::equal(m_data, other.m_data, Prisma::EPSILON));
#endif
    }

    bool operator!=(const Vector3& other) const {
        return !(*this == other);
    }

    // 成员函数
    float Length() const {
#if defined(PRISMA_USE_DIRECTXMATH)
        return DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(&m_data)));
#else
        return glm::length(m_data);
#endif
    }

    float LengthSquared() const {
#if defined(PRISMA_USE_DIRECTXMATH)
        return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&m_data)));
#else
        return glm::length2(m_data);
#endif
    }

    Vector3 Normalized() const {
        return *this / Length();
    }

    void Normalize() {
        *this = Normalized();
    }

    // 静态函数
    static float Distance(const Vector3& a, const Vector3& b) {
        return (a - b).Length();
    }

    static float DistanceSquared(const Vector3& a, const Vector3& b) {
        return (a - b).LengthSquared();
    }

    static float Dot(const Vector3& a, const Vector3& b) {
#if defined(PRISMA_USE_DIRECTXMATH)
        return DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMLoadFloat3(&a.m_data), DirectX::XMLoadFloat3(&b.m_data)));
#else
        return glm::dot(a.m_data, b.m_data);
#endif
    }

    static Vector3 Cross(const Vector3& a, const Vector3& b) {
#if defined(PRISMA_USE_DIRECTXMATH)
        return DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&a.m_data), DirectX::XMLoadFloat3(&b.m_data));
#else
        return Vector3(glm::cross(a.m_data, b.m_data));
#endif
    }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b - a) * t;
    }

    static Vector3 Slerp(const Vector3& a, const Vector3& b, float t) {
        // 转换为单位向量进行球面插值
        Vector3 na = a.Normalized();
        Vector3 nb = b.Normalized();
        float dot = Dot(na, nb);

        // 限制范围避免数值误差
        dot = glm::clamp(dot, -1.0f, 1.0f);
        float angle = acosf(dot);

        if (angle < Prisma::EPSILON) {
            return Lerp(a, b, t);
        }

        float sinAngle = sinf(angle);
        return (na * sinf((1.0f - t) * angle) + nb * sinf(t * angle)) / sinAngle;
    }

    static Vector3 Reflect(const Vector3& vector, const Vector3& normal) {
        return vector - normal * (2.0f * Dot(vector, normal));
    }

    static Vector3 Project(const Vector3& vector, const Vector3& onto) {
        return onto * (Dot(vector, onto) / Dot(onto, onto));
    }

    static Vector3 ProjectOnPlane(const Vector3& vector, const Vector3& planeNormal) {
        return vector - Project(vector, planeNormal);
    }

    // 角度相关
    static float Angle(const Vector3& a, const Vector3& b) {
        float dot = Dot(a.Normalized(), b.Normalized());
        return acosf(glm::clamp(dot, -1.0f, 1.0f));
    }

    // 预定义的静态向量
    const static Vector3 Zero;
    const static Vector3 One;
    const static Vector3 Up;
    const static Vector3 Down;
    const static Vector3 Left;
    const static Vector3 Right;
    const static Vector3 Forward;
    const static Vector3 Back;
};

// 全局运算符
inline Vector3 operator*(float scalar, const Vector3& vector) {
    return vector * scalar;
}

#endif //VECTOR3_H