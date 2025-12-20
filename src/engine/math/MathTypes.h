#pragma once

// 跨平台数学类型定义
// 根据平台选择不同的数学库实现

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

// 选择数学库
// 统一使用GLM的列主序约定，Vulkan兼容
#if defined(PRISMA_USE_GLM) || defined(PRISMA_PLATFORM_ANDROID) || defined(PRISMA_PLATFORM_OTHER)
    // 非Windows平台或指定使用GLM
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtc/quaternion.hpp>
    #include <glm/gtx/quaternion.hpp>
    #include <glm/gtc/type_ptr.hpp>
    // GLM默认是右手坐标系，列主序
    namespace PrismaMath = glm;

#elif defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM)
    // Windows平台使用DirectXMath，但需要转换到列主序
    #define PRISMA_USE_DIRECTXMATH 1
    #include <DirectXMath.h>
    namespace PrismaMath = DirectX;
    // 需要定义转换宏，将DirectXMath的行主序转换为列主序
    #define PRISMA_DIRECTXMATH_TO_COLUMN_MAJOR

#else
    // 默认使用GLM
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtc/quaternion.hpp>
    #include <glm/gtx/quaternion.hpp>
    #include <glm/gtc/type_ptr.hpp>
    namespace PrismaMath = glm;
#endif

// 统一的基本类型定义
namespace Prisma {

// 向量类型
#if defined(PRISMA_USE_DIRECTXMATH)
    using Vector2 = DirectX::XMFLOAT2;
    using Vector3 = DirectX::XMFLOAT3;
    using Vector4 = DirectX::XMFLOAT4;
    using IVector2 = DirectX::XMINT2;
    using IVector3 = DirectX::XMINT3;
    using IVector4 = DirectX::XMINT4;
    using UVector2 = DirectX::XMUINT2;
    using UVector3 = DirectX::XMUINT3;
    using UVector4 = DirectX::XMUINT4;
#else
    using Vector2 = glm::vec2;
    using Vector3 = glm::vec3;
    using Vector4 = glm::vec4;
    using IVector2 = glm::ivec2;
    using IVector3 = glm::ivec3;
    using IVector4 = glm::ivec4;
    using UVector2 = glm::uvec2;
    using UVector3 = glm::uvec3;
    using UVector4 = glm::uvec4;
#endif

// 矩阵类型
#if defined(PRISMA_USE_DIRECTXMATH)
    using Matrix3x3 = DirectX::XMFLOAT3X3;
    using Matrix4x4 = DirectX::XMFLOAT4X4;
#else
    using Matrix3x3 = glm::mat3;
    using Matrix4x4 = glm::mat4;
#endif

// 四元数类型
#if defined(PRISMA_USE_DIRECTXMATH)
    using Quaternion = DirectX::XMFLOAT4;
#else
    using Quaternion = glm::quat;
#endif

// 平面类型
#if defined(PRISMA_USE_DIRECTXMATH)
    using Plane = DirectX::XMFLOAT4;
#else
    struct Plane {
        Vector3 normal;
        float distance;

        Plane() : normal(0, 1, 0), distance(0) {}
        Plane(const Vector3& n, float d) : normal(n), distance(d) {}
        Plane(float a, float b, float c, float d) : normal(a, b, c), distance(d) {}
    };
#endif

// 颜色类型
#if defined(PRISMA_USE_DIRECTXMATH)
    using Color = DirectX::XMFLOAT4;
#else
    using Color = Vector4;
#endif

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