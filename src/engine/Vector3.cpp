//
// Created by JasonGu on 25-12-14.
//

#include "Vector3.h"
#include <DirectXMath.h>
#include <cmath>

// 静态成员定义
const Vector3 Vector3::Zero(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::One(1.0f, 1.0f, 1.0f);
const Vector3 Vector3::Up(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::Down(0.0f, -1.0f, 0.0f);
const Vector3 Vector3::Left(-1.0f, 0.0f, 0.0f);
const Vector3 Vector3::Right(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::Forward(0.0f, 0.0f, 1.0f);
const Vector3 Vector3::Back(0.0f, 0.0f, -1.0f);

// 构造函数
Vector3::Vector3() : m_vector(0.0f, 0.0f, 0.0f) {}

Vector3::Vector3(float x, float y, float z) : m_vector(x, y, z) {}

Vector3::Vector3(const DirectX::XMFLOAT3& vector) : m_vector(vector) {}

// 赋值操作符
Vector3& Vector3::operator=(const DirectX::XMFLOAT3& vector) {
    m_vector = vector;
    return *this;
}

// 数学运算符
Vector3 Vector3::operator+(const Vector3& other) const {
    return Vector3(
        m_vector.x + other.m_vector.x,
        m_vector.y + other.m_vector.y,
        m_vector.z + other.m_vector.z
    );
}

Vector3 Vector3::operator-(const Vector3& other) const {
    return Vector3(
        m_vector.x - other.m_vector.x,
        m_vector.y - other.m_vector.y,
        m_vector.z - other.m_vector.z
    );
}

Vector3 Vector3::operator*(float scalar) const {
    return Vector3(
        m_vector.x * scalar,
        m_vector.y * scalar,
        m_vector.z * scalar
    );
}

Vector3 Vector3::operator/(float scalar) const {
    return Vector3(
        m_vector.x / scalar,
        m_vector.y / scalar,
        m_vector.z / scalar
    );
}

// 复合赋值运算符
Vector3& Vector3::operator+=(const Vector3& other) {
    m_vector.x += other.m_vector.x;
    m_vector.y += other.m_vector.y;
    m_vector.z += other.m_vector.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& other) {
    m_vector.x -= other.m_vector.x;
    m_vector.y -= other.m_vector.y;
    m_vector.z -= other.m_vector.z;
    return *this;
}

Vector3& Vector3::operator*=(float scalar) {
    m_vector.x *= scalar;
    m_vector.y *= scalar;
    m_vector.z *= scalar;
    return *this;
}

Vector3& Vector3::operator/=(float scalar) {
    m_vector.x /= scalar;
    m_vector.y /= scalar;
    m_vector.z /= scalar;
    return *this;
}

// 比较运算符
bool Vector3::operator==(const Vector3& other) const {
    return m_vector.x == other.m_vector.x &&
           m_vector.y == other.m_vector.y &&
           m_vector.z == other.m_vector.z;
}

bool Vector3::operator!=(const Vector3& other) const {
    return !(*this == other);
}

// 成员函数
float Vector3::Length() const {
    return std::sqrt(LengthSquared());
}

float Vector3::LengthSquared() const {
    return m_vector.x * m_vector.x +
           m_vector.y * m_vector.y +
           m_vector.z * m_vector.z;
}

Vector3 Vector3::Normalized() const {
    Vector3 result(*this);
    result.Normalize();
    return result;
}

void Vector3::Normalize() {
    float length = Length();
    if (length > 0.0f) {
        float invLength = 1.0f / length;
        m_vector.x *= invLength;
        m_vector.y *= invLength;
        m_vector.z *= invLength;
    }
}

// 静态函数
float Vector3::Distance(const Vector3& a, const Vector3& b) {
    return (a - b).Length();
}

float Vector3::Dot(const Vector3& a, const Vector3& b) {
    return a.m_vector.x * b.m_vector.x +
           a.m_vector.y * b.m_vector.y +
           a.m_vector.z * b.m_vector.z;
}

Vector3 Vector3::Cross(const Vector3& a, const Vector3& b) {
    return Vector3(
        a.m_vector.y * b.m_vector.z - a.m_vector.z * b.m_vector.y,
        a.m_vector.z * b.m_vector.x - a.m_vector.x * b.m_vector.z,
        a.m_vector.x * b.m_vector.y - a.m_vector.y * b.m_vector.x
    );
}

Vector3 Vector3::Lerp(const Vector3& a, const Vector3& b, float t) {
    return Vector3(
        a.m_vector.x + t * (b.m_vector.x - a.m_vector.x),
        a.m_vector.y + t * (b.m_vector.y - a.m_vector.y),
        a.m_vector.z + t * (b.m_vector.z - a.m_vector.z)
    );
}
