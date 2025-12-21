#pragma once

// 跨平台数学类型定义
// 统一使用GLM以保持跨平台兼容性

// 平台检测宏
#if defined(ANDROID) || defined(__ANDROID__)
    #define PRISMA_PLATFORM_ANDROID 1
#elif defined(_WIN32) || defined(_WIN64)
    #define PRISMA_PLATFORM_WINDOWS 1
#else
    #define PRISMA_PLATFORM_OTHER 1
#endif

// 定义GLM实验性扩展支持
#define GLM_ENABLE_EXPERIMENTAL

// 所有平台统一使用GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// Windows平台可选：如果需要DirectXMath的性能优化，可以通过Prisma::Math命名空间使用
#ifdef PRISMA_PLATFORM_WINDOWS
    #include <DirectXMath.h>
    #define PRISMA_DIRECTXMATH_AVAILABLE 1
#endif

namespace PrismaMath = glm;

// 统一的基本类型定义
namespace Prisma {

// 向量类型 - 使用GLM
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using IVector2 = glm::ivec2;
using IVector3 = glm::ivec3;
using IVector4 = glm::ivec4;
using UVector2 = glm::uvec2;
using UVector3 = glm::uvec3;
using UVector4 = glm::uvec4;

// 矩阵类型
using Matrix3x3 = glm::mat3;
using Matrix4x4 = glm::mat4;

// 四元数类型
using Quaternion = glm::quat;

// 平面类型
struct Plane {
    Vector3 normal;
    float distance;

    Plane() : normal(0, 1, 0), distance(0) {}
    Plane(const Vector3& n, float d) : normal(n), distance(d) {}
    Plane(float a, float b, float c, float d) : normal(a, b, c), distance(d) {}
};

// 颜色类型
using Color = Vector4;

// 常用数学常量
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;
constexpr float QUARTER_PI = PI * 0.25f;
constexpr float INV_PI = 1.0f / PI;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// epsilon值
constexpr float EPSILON = 1e-6f;
constexpr float EPSILON_F = 1e-6f;

// 角度/弧度转换
constexpr float Deg2Rad(float degrees) { return degrees * DEG_TO_RAD; }
constexpr float Rad2Deg(float radians) { return radians * RAD_TO_DEG; }

} // namespace Prisma