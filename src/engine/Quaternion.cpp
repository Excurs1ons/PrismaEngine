#include "Quaternion.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

// 静态常量定义
const Quaternion Quaternion::Identity = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

// 构造函数
Quaternion::Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}

Quaternion::Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

Quaternion::Quaternion(const XMVECTOR& vector) {
    XMFLOAT4 float4;
    XMStoreFloat4(&float4, vector);
    this->x = float4.x;
    this->y = float4.y;
    this->z = float4.z;
    this->w = float4.w;
}

// 转换函数
XMVECTOR Quaternion::ToXMVector() const {
    return XMVectorSet(x, y, z, w);
}

void Quaternion::FromXMVector(const XMVECTOR& vector) {
    XMFLOAT4 float4;
    XMStoreFloat4(&float4, vector);
    x = float4.x;
    y = float4.y;
    z = float4.z;
    w = float4.w;
}

// 实用方法
void Quaternion::Normalize() {
    XMVECTOR vector = ToXMVector();
    vector = XMQuaternionNormalize(vector);
    FromXMVector(vector);
}

float Quaternion::Length() const {
    XMVECTOR vector = ToXMVector();
    XMVECTOR length = XMQuaternionLength(vector);
    return XMVectorGetX(length);
}

float Quaternion::LengthSquared() const {
    XMVECTOR vector = ToXMVector();
    XMVECTOR length = XMQuaternionLengthSq(vector);
    return XMVectorGetX(length);
}

Quaternion Quaternion::Normalized() const {
    Quaternion result(*this);
    result.Normalize();
    return result;
}

Quaternion Quaternion::Inverse() const {
    XMVECTOR vector = ToXMVector();
    XMVECTOR inverse = XMQuaternionInverse(vector);
    return Quaternion(inverse);
}

// 静态创建方法
Quaternion Quaternion::FromEulerAngles(float pitch, float yaw, float roll) {
    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(
        XMConvertToRadians(pitch),
        XMConvertToRadians(yaw),
        XMConvertToRadians(roll)
    );
    return Quaternion(quaternion);
}

Quaternion Quaternion::FromAxisAngle(const XMFLOAT3& axis, float angle) {
    XMVECTOR axisVector = XMLoadFloat3(&axis);
    XMVECTOR quaternion = XMQuaternionRotationAxis(axisVector, XMConvertToRadians(angle));
    return Quaternion(quaternion);
}

Quaternion Quaternion::FromRotationMatrix(const XMMATRIX& matrix) {
    XMVECTOR quaternion = XMQuaternionRotationMatrix(matrix);
    return Quaternion(quaternion);
}

// 球面线性插值
Quaternion Quaternion::Slerp(const Quaternion& a, const Quaternion& b, float t) {
    XMVECTOR vectorA = a.ToXMVector();
    XMVECTOR vectorB = b.ToXMVector();
    XMVECTOR result = XMQuaternionSlerp(vectorA, vectorB, t);
    return Quaternion(result);
}

// 运算符重载
Quaternion Quaternion::operator*(const Quaternion& other) const {
    XMVECTOR vectorA = ToXMVector();
    XMVECTOR vectorB = other.ToXMVector();
    XMVECTOR result = XMQuaternionMultiply(vectorA, vectorB);
    return Quaternion(result);
}

Quaternion& Quaternion::operator*=(const Quaternion& other) {
    XMVECTOR vectorA = ToXMVector();
    XMVECTOR vectorB = other.ToXMVector();
    XMVECTOR result = XMQuaternionMultiply(vectorA, vectorB);
    FromXMVector(result);
    return *this;
}

bool Quaternion::operator==(const Quaternion& other) const {
    return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
}

bool Quaternion::operator!=(const Quaternion& other) const {
    return !(*this == other);
}

// 辅助函数
float Quaternion::Clamp(float value, float min, float max) {
    if (value < min) {
        value = min;
    }
    if (value > max) {
        value = max;
    }
    return value;
}