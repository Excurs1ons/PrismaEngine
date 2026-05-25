#pragma once

#include "RenderTypes.h"
#include "IResource.h"

namespace Prisma::Graphic {

// 采样器抽象接口
class ISampler : public IResource {
public:
    virtual ~ISampler() = default;

    // 获取过滤模式
    virtual TextureFilter GetFilter() const = 0;

    // 获取U方向寻址模式
    virtual TextureAddressMode GetAddressU() const = 0;

    // 获取V方向寻址模式
    virtual TextureAddressMode GetAddressV() const = 0;

    // 获取W方向寻址模式
    virtual TextureAddressMode GetAddressW() const = 0;

    // 获取MIP LOD偏移
    virtual float GetMipLODBias() const = 0;

    // 获取最大各向异性
    virtual uint32_t GetMaxAnisotropy() const = 0;

    // 获取比较函数
    virtual TextureComparisonFunc GetComparisonFunc() const = 0;

    // 获取边框颜色
    virtual void GetBorderColor(float& r, float& g, float& b, float& a) const = 0;

    // 获取最小LOD
    virtual float GetMinLOD() const = 0;

    // 获取最大LOD
    virtual float GetMaxLOD() const = 0;

    // 获取采样器句柄
    virtual uint64_t GetHandle() const = 0;
};

} // namespace Prisma::Graphic