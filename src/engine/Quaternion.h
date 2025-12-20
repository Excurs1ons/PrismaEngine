// //
// // Created by JasonGu on 25-12-14.
// //
// #pragma once
//
// #ifndef QUATERNION_H
// #define QUATERNION_H
//
// #include "math/MathTypes.h"
//
// class Quaternion {
// public:
//     // 构造函数
//     Quaternion();
//     Quaternion(float x, float y, float z, float w);
//     Quaternion(const PrismaMath::vec4& vector);
//     Quaternion(const PrismaMath::vec3& euler); // 从欧拉角构造
//
//     // 静态常量
//     static const Quaternion Identity;
//
//     // 实用方法
//     void Normalize();
//     float Length() const;
//     float LengthSquared() const;
//     Quaternion Normalized() const;
//     Quaternion Inverse() const;
//
//     // 欧拉角转换
//     PrismaMath::vec3 ToEulerAngles() const;
//
//     // 点乘
//     float Dot(const Quaternion& other) const;
//
//     // 静态方法
//     static Quaternion IdentityQuaternion();
//     static Quaternion LookRotation(const PrismaMath::vec3& forward, const PrismaMath::vec3& up = PrismaMath::vec3(0, 1, 0));
//     static float Angle(const Quaternion& a, const Quaternion& b);
//
//     // 静态创建方法
//     static Quaternion FromEulerAngles(float pitch, float yaw, float roll);
//     static Quaternion FromAxisAngle(const PrismaMath::vec3& axis, float angle);
//     static Quaternion FromRotationMatrix(const PrismaMath::mat4& matrix);
//
//     // 球面线性插值
//     static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t);
//
//     // 运算符重载
//     Quaternion operator*(const Quaternion& other) const;
//     Quaternion& operator*=(const Quaternion& other);
//     bool operator==(const Quaternion& other) const;
//     bool operator!=(const Quaternion& other) const;
//
//     // 成员变量
//     float x, y, z, w;
//
// private:
//     // 平台特定的内部数据
// #if defined(PRISMA_USE_DIRECTXMATH)
//     DirectX::XMFLOAT4 m_internalData;
// #endif
//
//     // 辅助函数
//     static float Clamp(float value, float min, float max);
//
//     // 平台特定的转换函数
//     PrismaMath::vec4 ToInternalVector() const;
//     void FromInternalVector(const PrismaMath::vec4& vector);
// };
//
// #endif //QUATERNION_H