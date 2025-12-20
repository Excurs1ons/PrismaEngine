// #include "Quaternion.h"
// #include "math/Math.h"
// #include <cmath>
//
// // 静态常量定义
// const Quaternion Quaternion::Identity = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
//
// // 构造函数
// Quaternion::Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
//
// Quaternion::Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
//
// Quaternion::Quaternion(const PrismaMath::vec4& vector) {
//     this->x = vector.x;
//     this->y = vector.y;
//     this->z = vector.z;
//     this->w = vector.w;
// }
//
// Quaternion::Quaternion(const PrismaMath::vec3& euler) {
//     *this = FromEulerAngles(euler.x, euler.y, euler.z);
// }
//
// // 平台特定的转换函数
// PrismaMath::vec4 Quaternion::ToInternalVector() const {
//     return PrismaMath::vec4(x, y, z, w);
// }
//
// void Quaternion::FromInternalVector(const PrismaMath::vec4& vector) {
//     x = vector.x;
//     y = vector.y;
//     z = vector.z;
//     w = vector.w;
// }
//
// // 实用方法
// void Quaternion::Normalize() {
//     PrismaMath::vec4 q = ToInternalVector();
//     q = Prisma::Normalize(q);
//     FromInternalVector(q);
// }
//
// float Quaternion::Length() const {
//     PrismaMath::vec4 q = ToInternalVector();
//     return Prisma::Length(q);
// }
//
// float Quaternion::LengthSquared() const {
//     PrismaMath::vec4 q = ToInternalVector();
//     return Prisma::LengthSquared(q);
// }
//
// Quaternion Quaternion::Normalized() const {
//     Quaternion result(*this);
//     result.Normalize();
//     return result;
// }
//
// Quaternion Quaternion::Inverse() const {
//     PrismaMath::vec4 q = ToInternalVector();
//     PrismaMath::vec4 inverse = Prisma::Inverse(q);
//     return Quaternion(inverse);
// }
//
// // 静态创建方法
// Quaternion Quaternion::FromEulerAngles(float pitch, float yaw, float roll) {
//     // 转换为弧度
//     float radPitch = Prisma::Radians(pitch);
//     float radYaw = Prisma::Radians(yaw);
//     float radRoll = Prisma::Radians(roll);
//
//     PrismaMath::vec4 quaternion = Prisma::FromEulerAngles(radPitch, radYaw, radRoll);
//     return Quaternion(quaternion);
// }
//
// Quaternion Quaternion::FromAxisAngle(const PrismaMath::vec3& axis, float angle) {
//     float radAngle = Prisma::Radians(angle);
//     PrismaMath::vec4 quaternion = Prisma::FromAxisAngle(axis, radAngle);
//     return Quaternion(quaternion);
// }
//
// Quaternion Quaternion::FromRotationMatrix(const PrismaMath::mat4& matrix) {
//     PrismaMath::vec4 quaternion = Prisma::FromRotationMatrix(matrix);
//     return Quaternion(quaternion);
// }
//
// // 球面线性插值
// Quaternion Quaternion::Slerp(const Quaternion& a, const Quaternion& b, float t) {
//     PrismaMath::vec4 q1 = a.ToInternalVector();
//     PrismaMath::vec4 q2 = b.ToInternalVector();
//     PrismaMath::vec4 result = Prisma::Slerp(q1, q2, t);
//     return Quaternion(result);
// }
//
// // 运算符重载
// Quaternion Quaternion::operator*(const Quaternion& other) const {
//     PrismaMath::vec4 q1 = ToInternalVector();
//     PrismaMath::vec4 q2 = other.ToInternalVector();
//     PrismaMath::vec4 result = Prisma::Multiply(q1, q2);
//     return Quaternion(result);
// }
//
// Quaternion& Quaternion::operator*=(const Quaternion& other) {
//     PrismaMath::vec4 q1 = ToInternalVector();
//     PrismaMath::vec4 q2 = other.ToInternalVector();
//     PrismaMath::vec4 result = Prisma::Multiply(q1, q2);
//     FromInternalVector(result);
//     return *this;
// }
//
// bool Quaternion::operator==(const Quaternion& other) const {
//     return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
// }
//
// bool Quaternion::operator!=(const Quaternion& other) const {
//     return !(*this == other);
// }
//
// // 欧拉角转换
// PrismaMath::vec3 Quaternion::ToEulerAngles() const {
//     return Prisma::ToEulerAngles(ToInternalVector());
// }
//
// // 点乘
// float Quaternion::Dot(const Quaternion& other) const {
//     PrismaMath::vec4 q1 = ToInternalVector();
//     PrismaMath::vec4 q2 = other.ToInternalVector();
//     return Prisma::Dot(q1, q2);
// }
//
// // 静态方法
// Quaternion Quaternion::IdentityQuaternion() {
//     return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
// }
//
// Quaternion Quaternion::LookRotation(const PrismaMath::vec3& forward, const PrismaMath::vec3& up) {
//     PrismaMath::vec4 quaternion = Prisma::LookRotation(forward, up);
//     return Quaternion(quaternion);
// }
//
// float Quaternion::Angle(const Quaternion& a, const Quaternion& b) {
//     PrismaMath::vec4 q1 = a.ToInternalVector();
//     PrismaMath::vec4 q2 = b.ToInternalVector();
//
//     // 计算点积
//     float dot = Prisma::Dot(q1, q2);
//     dot = Clamp(dot, -1.0f, 1.0f);
//
//     // 计算角度
//     return Prisma::Degrees(acosf(fabsf(dot)) * 2.0f);
// }
//
// // 辅助函数
// float Quaternion::Clamp(float value, float min, float max) {
//     if (value < min) {
//         value = min;
//     }
//     if (value > max) {
//         value = max;
//     }
//     return value;
// }