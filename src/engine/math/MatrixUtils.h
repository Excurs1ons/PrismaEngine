#pragma once

#include "MathTypes.h"

namespace PrismaEngine {
namespace Math {

// DirectXMath行主序到Vulkan列主序的转换工具类
struct MatrixUtils {
#if defined(PRISMA_DIRECTXMATH_TO_COLUMN_MAJOR)
    // DirectXMath XMMATRIX (行主序) 转换为 Vulkan 列主序矩阵
    static Matrix4x4 DirectXToVulkan(const DirectX::XMMATRIX& dxMatrix) {
        Matrix4x4 result;
        DirectX::XMFLOAT4X4 temp;
        DirectX::XMStoreFloat4x4(&temp, dxMatrix);

        // 转置矩阵：行主序 -> 列主序
        result._11 = temp._11; result._12 = temp._21; result._13 = temp._31; result._14 = temp._41;
        result._21 = temp._12; result._22 = temp._22; result._23 = temp._32; result._24 = temp._42;
        result._31 = temp._13; result._32 = temp._23; result._33 = temp._33; result._34 = temp._43;
        result._41 = temp._14; result._42 = temp._24; result._43 = temp._34; result._44 = temp._44;

        return result;
    }

    // Vulkan 列主序矩阵转换为 DirectXMath XMMATRIX (行主序)
    static DirectX::XMMATRIX VulkanToDirectX(const Matrix4x4& vkMatrix) {
        DirectX::XMFLOAT4X4 temp;

        // 转置矩阵：列主序 -> 行主序
        temp._11 = vkMatrix._11; temp._12 = vkMatrix._21; temp._13 = vkMatrix._31; temp._14 = vkMatrix._41;
        temp._21 = vkMatrix._12; temp._22 = vkMatrix._22; temp._23 = vkMatrix._32; temp._24 = vkMatrix._42;
        temp._31 = vkMatrix._13; temp._32 = vkMatrix._23; temp._33 = vkMatrix._33; temp._34 = vkMatrix._43;
        temp._41 = vkMatrix._14; temp._42 = vkMatrix._24; temp._43 = vkMatrix._34; temp._44 = vkMatrix._44;

        return DirectX::XMLoadFloat4x4(&temp);
    }

    // 从行主序 XMFLOAT4X4 创建列主序 Matrix4x4
    static Matrix4x4 CreateFromRowMajor(const DirectX::XMFLOAT4X4& rowMajor) {
        Matrix4x4 result;
        result._11 = rowMajor._11; result._12 = rowMajor._21; result._13 = rowMajor._31; result._14 = rowMajor._41;
        result._21 = rowMajor._12; result._22 = rowMajor._22; result._23 = rowMajor._32; result._24 = rowMajor._42;
        result._31 = rowMajor._13; result._32 = rowMajor._23; result._33 = rowMajor._33; result._34 = rowMajor._43;
        result._41 = rowMajor._14; result._42 = rowMajor._24; result._43 = rowMajor._34; result._44 = rowMajor._44;
        return result;
    }

    // 将列主序 Matrix4x4 转换为行主序 XMFLOAT4X4
    static DirectX::XMFLOAT4X4 ToRowMajor(const Matrix4x4& columnMajor) {
        DirectX::XMFLOAT4X4 result;
        result._11 = columnMajor._11; result._12 = columnMajor._21; result._13 = columnMajor._31; result._14 = columnMajor._41;
        result._21 = columnMajor._12; result._22 = columnMajor._22; result._23 = columnMajor._32; result._24 = columnMajor._42;
        result._31 = columnMajor._13; result._32 = columnMajor._23; result._33 = columnMajor._33; result._34 = columnMajor._43;
        result._41 = columnMajor._14; result._42 = columnMajor._24; result._43 = columnMajor._34; result._44 = columnMajor._44;
        return result;
    }
#else
    // GLM已经是列主序，无需转换
    static Matrix4x4 DirectXToVulkan(const Matrix4x4& matrix) { return matrix; }
    static Matrix4x4 VulkanToDirectX(const Matrix4x4& matrix) { return matrix; }
#endif

    // 创建透视投影矩阵（Vulkan标准，右手坐标系）
    static Matrix4x4 CreatePerspective(float fov, float aspect, float near, float far) {
#if defined(PRISMA_USE_DIRECTXMATH)
        // DirectX的PerspectiveFovLH是左手坐标系，需要调整
        DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, near, far);
        // 转换为右手坐标系：反转Z轴
        DirectX::XMMATRIX flipZ = DirectX::XMMatrixScaling(1.0f, 1.0f, -1.0f);
        proj = XMMatrixMultiply(proj, flipZ);
        return DirectXToVulkan(proj);
#else
        // GLM的perspective是右手坐标系，但需要调整近远平面
        return glm::perspectiveRH(fov, aspect, near, far);
#endif
    }

    // 创建正交投影矩阵
    static Matrix4x4 CreateOrthographic(float left, float right, float bottom, float top, float near, float far) {
#if defined(PRISMA_USE_DIRECTXMATH)
        DirectX::XMMATRIX proj = DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, near, far);
        // 转换为右手坐标系
        DirectX::XMMATRIX flipZ = DirectX::XMMatrixScaling(1.0f, 1.0f, -1.0f);
        proj = XMMatrixMultiply(proj, flipZ);
        return DirectXToVulkan(proj);
#else
        return glm::orthoRH(left, right, bottom, top, near, far);
#endif
    }

    // 创建视图矩阵（右手坐标系）
    static Matrix4x4 CreateLookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
#if defined(PRISMA_USE_DIRECTXMATH)
        // DirectX的LookAtLH是左手坐标系，需要使用RH版本
        DirectX::XMVECTOR eyeVec = DirectX::XMLoadFloat3(&eye.m_data);
        DirectX::XMVECTOR targetVec = DirectX::XMLoadFloat3(&target.m_data);
        DirectX::XMVECTOR upVec = DirectX::XMLoadFloat3(&up.m_data);
        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtRH(eyeVec, targetVec, upVec);
        return DirectXToVulkan(view);
#else
        return glm::lookAtRH(eye, target, up);
#endif
    }
};

} // namespace Math
} // namespace PrismaEngine