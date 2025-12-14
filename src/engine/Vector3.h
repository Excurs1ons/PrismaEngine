//
// Created by JasonGu on 25-12-14.
//

#ifndef VECTOR3_H
#define VECTOR3_H

#include <DirectXMath.h>

// 包装XMFLOAT3
struct Vector3 {
    DirectX::XMFLOAT3 m_vector;
    
    // 构造函数
    Vector3();
    Vector3(float x, float y, float z);
    Vector3(const DirectX::XMFLOAT3& vector);
    
    // 隐式转换操作符
    operator DirectX::XMFLOAT3() const { return m_vector; }
    
    // 赋值操作符
    Vector3& operator=(const DirectX::XMFLOAT3& vector);
    
    // 数学运算符
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(float scalar) const;
    Vector3 operator/(float scalar) const;
    
    // 复合赋值运算符
    Vector3& operator+=(const Vector3& other);
    Vector3& operator-=(const Vector3& other);
    Vector3& operator*=(float scalar);
    Vector3& operator/=(float scalar);
    
    // 比较运算符
    bool operator==(const Vector3& other) const;
    bool operator!=(const Vector3& other) const;
    
    // 成员函数
    float Length() const;
    float LengthSquared() const;
    Vector3 Normalized() const;
    void Normalize();
    
    // 静态函数
    static float Distance(const Vector3& a, const Vector3& b);
    static float Dot(const Vector3& a, const Vector3& b);
    static Vector3 Cross(const Vector3& a, const Vector3& b);
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t);
    
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

#endif //VECTOR3_H
