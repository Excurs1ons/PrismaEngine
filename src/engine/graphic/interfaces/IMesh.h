#pragma once

#include "RenderTypes.h"
#include "IBuffer.h"
#include <vector>
#include <memory>
#include <string>

namespace Prisma::Graphic {

// 网格抽象接口
class IMesh {
public:
    virtual ~IMesh() = default;

    // 获取子网格数量
    virtual uint32_t GetSubMeshCount() const = 0;

    // 获取子网格
    virtual const SubMeshBuffer* GetSubMesh(uint32_t index) const = 0;

    // 添加子网格
    virtual uint32_t AddSubMesh(const SubMeshBuffer& subMesh) = 0;

    // 获取全局包围盒
    virtual const BoundingBox& GetBoundingBox() const = 0;

    // 更新包围盒
    virtual void UpdateBoundingBox() = 0;

    // 绑定网格到渲染管线
    virtual void Bind(class ICommandBuffer* commandBuffer, uint32_t subMeshIndex = 0) = 0;

    // 绘制网格
    virtual void Draw(class ICommandBuffer* commandBuffer, uint32_t subMeshIndex = 0) = 0;

    // 绘制实例化网格
    virtual void DrawInstanced(class ICommandBuffer* commandBuffer, uint32_t instanceCount, uint32_t subMeshIndex = 0) = 0;

    // 获取网格名称
    virtual const std::string& GetName() const = 0;

    // 设置网格名称
    virtual void SetName(const std::string& name) = 0;

    // 是否保留CPU数据
    virtual void SetKeepCpuData(bool keep) = 0;

    // 是否已经上传到GPU
    virtual bool IsUploaded() const = 0;

    // 上传到GPU
    virtual bool UploadToGPU(class IRenderDevice* device) = 0;

    // 从GPU卸载
    virtual void UnloadFromGPU() = 0;
};

} // namespace Prisma::Graphic