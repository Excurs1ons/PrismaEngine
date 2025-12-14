//
// Created by JasonGu on 25-12-14.
//
#pragma once

#ifndef QUATERNION_H
#define QUATERNION_H
#include <DirectXMath.h>

class Quaternion {
public:
    // 构造函数
    Quaternion();
    Quaternion(float x, float y, float z, float w);
    Quaternion(const DirectX::XMVECTOR& vector);

    // 静态常量
    static const Quaternion Identity;

    // 转换函数
    DirectX::XMVECTOR ToXMVector() const;
    void FromXMVector(const DirectX::XMVECTOR& vector);

    // 实用方法
    void Normalize();
    float Length() const;
    float LengthSquared() const;
    Quaternion Normalized() const;
    Quaternion Inverse() const;

    // 静态创建方法
    static Quaternion FromEulerAngles(float pitch, float yaw, float roll);
    static Quaternion FromAxisAngle(const DirectX::XMFLOAT3& axis, float angle);
    static Quaternion FromRotationMatrix(const DirectX::XMMATRIX& matrix);

    // 球面线性插值
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t);

    // 运算符重载
    Quaternion operator*(const Quaternion& other) const;
    Quaternion& operator*=(const Quaternion& other);
    bool operator==(const Quaternion& other) const;
    bool operator!=(const Quaternion& other) const;

    // 成员变量
    float x, y, z, w;

private:
    // 辅助函数
    static float Clamp(float value, float min, float max);
};

#endif //QUATERNION_H