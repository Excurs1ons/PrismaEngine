#pragma once

#include "IResource.h"
#include "RenderTypes.h"

namespace PrismaEngine::Graphic {

/// @brief 采样器抽象接口
class ISampler : public IResource {
public:
    virtual ~ISampler() = default;

    /// @brief 获取过滤模式
    /// @return 过滤模式
    virtual TextureFilter GetFilter() const = 0;

    /// @brief 获取U方向寻址模式
    /// @return U方向寻址模式
    virtual TextureAddressMode GetAddressU() const = 0;

    /// @brief 获取V方向寻址模式
    /// @return V方向寻址模式
    virtual TextureAddressMode GetAddressV() const = 0;

    /// @brief 获取W方向寻址模式
    /// @return W方向寻址模式
    virtual TextureAddressMode GetAddressW() const = 0;

    /// @brief 获取MIP LOD偏移
    /// @return MIP LOD偏移
    virtual float GetMipLODBias() const = 0;

    /// @brief 获取最大各向异性
    /// @return 最大各向异性
    virtual uint32_t GetMaxAnisotropy() const = 0;

    /// @brief 获取比较函数
    /// @return 比较函数
    virtual TextureComparisonFunc GetComparisonFunc() const = 0;

    /// @brief 获取边框颜色
    /// @param[out] r 红色分量
    /// @param[out] g 绿色分量
    /// @param[out] b 蓝色分量
    /// @param[out] a Alpha分量
    virtual void GetBorderColor(float& r, float& g, float& b, float& a) const = 0;

    /// @brief 获取最小LOD
    /// @return 最小LOD
    virtual float GetMinLOD() const = 0;

    /// @brief 获取最大LOD
    /// @return 最大LOD
    virtual float GetMaxLOD() const = 0;

    /// @brief 获取采样器句柄
    /// @return 句柄
    virtual uint64_t GetHandle() const = 0;
};

} // namespace PrismaEngine::Graphic