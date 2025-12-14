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

Quaternion::Quaternion(const XMFLOAT3& euler) {
    *this = FromEulerAngles(euler.x, euler.y, euler.z);
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

// 欧拉角转换
XMFLOAT3 Quaternion::ToEulerAngles() const {
    XMFLOAT3 euler;
    XMVECTOR q = ToXMVector();

    // 提取欧拉角 (roll, pitch, yaw)
    float test = q.m128_f32[0] * q.m128_f32[1] + q.m128_f32[2] * q.m128_f32[3];
    if (test > 0.499f) {
        // 奇点处理：north pole
        euler.y = 2.0f * atan2f(q.m128_f32[0], q.m128_f32[3]);
        euler.x = XM_PIDIV2;
        euler.z = 0;
    } else if (test < -0.499f) {
        // 奇点处理：south pole
        euler.y = -2.0f * atan2f(q.m128_f32[0], q.m128_f32[3]);
        euler.x = -XM_PIDIV2;
        euler.z = 0;
    } else {
        float sqx = q.m128_f32[0] * q.m128_f32[0];
        float sqy = q.m128_f32[1] * q.m128_f32[1];
        float sqz = q.m128_f32[2] * q.m128_f32[2];
        euler.y = atan2f(2.0f * q.m128_f32[1] * q.m128_f32[3] - 2.0f * q.m128_f32[0] * q.m128_f32[2],
                         1.0f - 2.0f * (sqy + sqz));
        euler.x = asinf(2.0f * test);
        euler.z = atan2f(2.0f * q.m128_f32[0] * q.m128_f32[3] - 2.0f * q.m128_f32[1] * q.m128_f32[2],
                         1.0f - 2.0f * (sqx + sqz));
    }

    // 转换为度数
    euler.x = XMConvertToDegrees(euler.x);
    euler.y = XMConvertToDegrees(euler.y);
    euler.z = XMConvertToDegrees(euler.z);

    return euler;
}

// 点乘
float Quaternion::Dot(const Quaternion& other) const {
    XMVECTOR q1 = ToXMVector();
    XMVECTOR q2 = other.ToXMVector();
    XMVECTOR dot = XMVector4Dot(q1, q2);
    return XMVectorGetX(dot);
}

// 静态方法
Quaternion Quaternion::IdentityQuaternion() {
    return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
}

Quaternion Quaternion::LookRotation(const XMFLOAT3& forward, const XMFLOAT3& up) {
    XMVECTOR f = XMLoadFloat3(&forward);
    XMVECTOR u = XMLoadFloat3(&up);

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
    return Quaternion(rotationQuat);
}

float Quaternion::Angle(const Quaternion& a, const Quaternion& b) {
    XMVECTOR q1 = a.ToXMVector();
    XMVECTOR q2 = b.ToXMVector();

    // 计算点积
    float dot = XMVectorGetX(XMVector4Dot(q1, q2));
    dot = Clamp(dot, -1.0f, 1.0f);

    // 计算角度
    return XMConvertToDegrees(acosf(fabsf(dot)) * 2.0f);
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