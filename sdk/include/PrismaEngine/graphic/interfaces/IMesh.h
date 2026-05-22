#pragma once

#include "RenderTypes.h"
#include "IBuffer.h"
#include <vector>
#include <memory>
#include <string>

namespace Prisma::Graphic {

/// @brief 网格抽象接口
class IMesh {
public:
    virtual ~IMesh() = default;

    /// @brief 获取子网格数量
    /// @return 子网格数量
    virtual uint32_t GetSubMeshCount() const = 0;

    /// @brief 获取子网格
    /// @param index 子网格索引
    /// @return 子网格数据
    virtual const SubMeshBuffer* GetSubMesh(uint32_t index) const = 0;

    /// @brief 添加子网格
    /// @param subMesh 子网格数据
    /// @return 子网格索引
    virtual uint32_t AddSubMesh(const SubMeshBuffer& subMesh) = 0;

    /// @brief 获取全局包围盒
    /// @return 包围盒
    virtual const BoundingBox& GetBoundingBox() const = 0;

    /// @brief 更新包围盒
    virtual void UpdateBoundingBox() = 0;

    /// @brief 绑定网格到渲染管线
    /// @param commandBuffer 命令缓冲区
    /// @param subMeshIndex 子网格索引
    virtual void Bind(class ICommandBuffer* commandBuffer, uint32_t subMeshIndex = 0) = 0;

    /// @brief 绘制网格
    /// @param commandBuffer 命令缓冲区
    /// @param subMeshIndex 子网格索引
    virtual void Draw(class ICommandBuffer* commandBuffer, uint32_t subMeshIndex = 0) = 0;

    /// @brief 绘制实例化网格
    /// @param commandBuffer 命令缓冲区
    /// @param instanceCount 实例数量
    /// @param subMeshIndex 子网格索引
    virtual void DrawInstanced(class ICommandBuffer* commandBuffer, uint32_t instanceCount, uint32_t subMeshIndex = 0) = 0;

    /// @brief 获取网格名称
    /// @return 网格名称
    virtual const std::string& GetName() const = 0;

    /// @brief 设置网格名称
    /// @param name 网格名称
    virtual void SetName(const std::string& name) = 0;

    /// @brief 是否保留CPU数据
    /// @param keep 是否保留
    virtual void SetKeepCpuData(bool keep) = 0;

    /// @brief 是否已经上传到GPU
    /// @return 是否在GPU上
    virtual bool IsUploaded() const = 0;

    /// @brief 上传到GPU
    /// @param device 渲染设备
    /// @return 是否成功
    virtual bool UploadToGPU(class IRenderDevice* device) = 0;

    /// @brief 从GPU卸载
    virtual void UnloadFromGPU() = 0;
};

} // namespace Prisma::Graphic