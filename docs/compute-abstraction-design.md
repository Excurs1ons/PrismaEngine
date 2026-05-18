# 计算着色器抽象层设计

## 概述

在 Template3D 的路径追踪示例中，约 40 个原生 Vulkan API 调用直连了 `vkCreateComputePipelines`、`vkCmdDispatch`、`vmaCreateImage` 等接口。本文档定义了完整的抽象层，使 Template3D 通过 Engine API（`IRenderDevice`、`IResourceFactory`、`ICommandBuffer` 等）完成所有 GPU 操作，零原生 Vulkan 调用。

### 设计目标

1. **完全抽象** — Template3D 不出现 `VkImage`/`VkBuffer`/`VkPipeline`/`VkDescriptorSet` 等原生类型
2. **增量迁移** — 每个模块可独立实现，不破坏现有图形管线
3. **向后兼容** — 已有接口签名不变，只做扩展
4. **后端无关** — 接口设计不泄露 Vulkan/DX12 实现细节

### 现有基础设施

以下能力已存在于接口层，直接可用：

| 能力 | 位置 | 状态 |
|------|------|------|
| `PipelineType::Compute` | `RenderTypes.h:479` | ✅ 已定义 |
| `ShaderType::Compute` | `RenderTypes.h:243` | ✅ 已定义 |
| `ICommandBuffer::Dispatch(x,y,z)` | `ICommandBuffer.h` | ✅ 已实现 |
| `IBuffer::Map()/Unmap()` | `IBuffer.h` | ✅ 已实现 |
| `BufferType::Structured` | `RenderTypes.h` | ✅ 已定义 |
| `BufferUsage::UnorderedAccess` | `RenderTypes.h` | ✅ 已定义 |
| `TextureDesc::allowUnorderedAccess` | `ITexture.h` | ✅ 已定义 |
| `ITexture::CreateDescriptor(UnorderedAccessView)` | `ITexture.h` | ✅ 已定义 |
| 管线创建流程 (`CreatePipelineStateImpl → SetShader → Create`) | `IPipelineState.h` | ✅ 已实现 |

---

## 模块 1：计算管线接口 `IComputePipeline`

### 问题

`IPipelineState` 包含图形专用字段（光栅化器、混合、深度模板、顶点输入），计算管线不需要这些。当前无法在 `IPipelineState` 上干净地表示计算管线。

### 设计

新增独立接口 `IComputePipeline`，只包含计算所需的属性和方法。

```cpp
// 文件: src/engine/graphic/interfaces/IComputePipeline.h (新建)

namespace Prisma::Graphic {

/**
 * @brief 计算管线抽象接口
 * 代表一个编译好的计算管线状态对象 (Compute PSO)
 * 与 IPipelineState 分离，避免图形专用字段污染计算管线
 */
class IComputePipeline {
public:
    virtual ~IComputePipeline() = default;

    /// @brief 获取管线类型
    [[nodiscard]] virtual PipelineType GetType() const = 0;

    /// @brief 获取管线状态
    [[nodiscard]] virtual bool IsValid() const = 0;

    // === 着色器管理 ===

    /// @brief 设置计算着色器
    /// @param shader 计算着色器对象
    virtual void SetShader(std::shared_ptr<IShader> shader) = 0;

    /// @brief 获取计算着色器
    [[nodiscard]] virtual std::shared_ptr<IShader> GetShader() const = 0;

    // === 推送常量 ===

    /// @brief 设置推送常量范围
    /// @param size 推送常量大小（字节）
    /// @param offset 推送常量偏移量
    virtual void SetPushConstantRange(uint32_t size, uint32_t offset = 0) = 0;

    /// @brief 获取推送常量大小
    [[nodiscard]] virtual uint32_t GetPushConstantRangeSize() const = 0;

    /// @brief 获取推送常量偏移量
    [[nodiscard]] virtual uint32_t GetPushConstantRangeOffset() const = 0;

    // === 描述符布局 ===

    /// @brief 获取描述符集布局列表
    /// @return 描述符集布局数组
    [[nodiscard]] virtual const std::vector<std::shared_ptr<IDescriptorSetLayout>>&
        GetDescriptorSetLayouts() const = 0;

    // === 生命周期 ===

    /// @brief 编译/创建计算管线
    /// @param device 渲染设备
    /// @return 是否成功
    virtual bool Create(IRenderDevice* device) = 0;

    // === 调试 ===

    virtual void SetDebugName(const std::string& name) = 0;
    virtual const std::string& GetDebugName() const = 0;
};

} // namespace Prisma::Graphic
```

### 资源工厂扩展

在 `IResourceFactory` 中新增工厂方法：

```cpp
// IResourceFactory.h 新增
virtual std::unique_ptr<IComputePipeline> CreateComputePipelineImpl() = 0;
```

### 命令缓冲区扩展

在 `ICommandBuffer` 中新增绑定方法：

```cpp
// ICommandBuffer.h 新增
virtual void SetComputePipeline(IComputePipeline* pipeline) = 0;
```

### 后端实现要点（Vulkan）

| 步骤 | 说明 |
|------|------|
| `VulkanComputePipeline` | 新类继承 `IComputePipeline` |
| `SetShader()` | 保存 `VulkanShader` 引用，提取 SPIR-V |
| `SetPushConstantRange()` | 配置 `VkPushConstantRange` |
| `Create()` | 构造 `VkComputePipelineCreateInfo`，调用 `vkCreateComputePipelines` |
| `GetDescriptorSetLayouts()` | 从着色器反射中提取布局信息 |
| `VulkanResourceFactory::CreateComputePipelineImpl()` | 返回 `VulkanComputePipeline` |
| `VulkanCommandBuffer::SetComputePipeline()` | 调用 `vkCmdBindPipeline(pipeline, VK_PIPELINE_BIND_POINT_COMPUTE)` |

### Template3D 使用示例

```cpp
// 创建计算管线
auto computePipeline = resourceFactory->CreateComputePipelineImpl();
computePipeline->SetShader(computeShader);
computePipeline->Create(device);

// 每帧调度
cmdBuffer->SetComputePipeline(computePipeline.get());
cmdBuffer->BindDescriptorSet(0, descriptorSet.get());
cmdBuffer->Dispatch((width + 7) / 8, (height + 7) / 8, 1);
```

---

## 模块 2：描述符绑定类型区分

### 问题

`IDescriptorSet::BindBuffer(binding, buffer, offset, size)` 不区分 UBO 和 SSBO。Vulkan 在 `vkUpdateDescriptorSets` 时需要指定 `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` 或 `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`，当前接口无法传递此信息。

### 设计

在 `BindBuffer` 中增加 `DescriptorType` 参数，让调用方显式指定绑定类型。

```cpp
// 文件: src/engine/graphic/interfaces/IDescriptorSet.h

/// @brief 描述符类型枚举（新增）
enum class DescriptorType {
    UniformBuffer,      // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    StorageBuffer,      // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER (SSBO)
    StorageImage,       // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    SampledImage,       // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    Sampler,            // VK_DESCRIPTOR_TYPE_SAMPLER
};

// IDescriptorSet 变更
class IDescriptorSet {
public:
    // 原有 BindTexture 签名不变
    virtual void BindTexture(uint32_t binding, ITexture* texture, ISampler* sampler) = 0;

    // BindBuffer 增加 DescriptorType 参数（默认 UniformBuffer 保持向后兼容）
    virtual void BindBuffer(uint32_t binding, IBuffer* buffer,
                            uint32_t offset, uint32_t size,
                            DescriptorType type = DescriptorType::UniformBuffer) = 0;

    // BindTexture 增加 StorageImage 重载（可选，便于显式表达意图）
    virtual void BindStorageImage(uint32_t binding, ITexture* texture) = 0;
};
```

### 后端实现要点（Vulkan）

| 步骤 | 说明 |
|------|------|
| `BindBuffer(type=UniformBuffer)` | 写入 `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` |
| `BindBuffer(type=StorageBuffer)` | 写入 `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER` |
| `BindStorageImage()` | 写入 `VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`，使用 `VkDescriptorImageInfo` |

### 替代方案对比

| 方案 | 优点 | 缺点 |
|------|------|------|
| **A: 显式传参（选定）** | 清晰、零歧义、调用方可控 | 调用方需知道类型 |
| B: 从布局隐式推断 | 调用方更简洁 | 需要布局信息运行时查询，增加复杂度 |

### Template3D 使用示例

```cpp
// 当前 Vulkan 代码
VkDescriptorBufferInfo cameraUBOInfo = { cameraUBO, 0, sizeof(CameraUBO) };
VkDescriptorBufferInfo sceneSSBOInfo = { sceneSSBO, 0, sceneDataSize };
VkWriteDescriptorSet writes[4];
writes[0].dstBinding = 2;  // UBO
writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
writes[1].dstBinding = 3;  // SSBO
writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

// 改后代码
descriptorSet->BindBuffer(2, cameraUBO, 0, sizeof(CameraUBO), DescriptorType::UniformBuffer);
descriptorSet->BindBuffer(3, sceneSSBO, 0, sceneDataSize, DescriptorType::StorageBuffer);
descriptorSet->BindStorageImage(0, outputStorageImage);
```

---

## 模块 3：Storage Image 支持

### 问题

Template3D 通过 `vmaCreateImage` + `VK_IMAGE_USAGE_STORAGE_BIT` + `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` 创建存储图像，然后通过 `vkCreateImageView` 创建视图。当前引擎若 `TextureDesc::allowUnorderedAccess = true`，需要确保 Vulkan 后端正确设置这些标志。

### 设计

接口层改动极小，主要是 **Vulkan 后端实现完善**。

#### 接口层面

`TextureDesc` 已有 `allowUnorderedAccess`，无需改动结构体。但需要确保纹理描述中能传递计算所需的大小信息——已通过 `width`/`height` 覆盖。

#### Vulkan 后端实现

```cpp
// VulkanResourceFactory::CreateTextureImpl()
VkImageCreateInfo imageInfo = { ... };
if (desc.allowUnorderedAccess) {
    imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    // 选择支持 VK_IMAGE_TILING_OPTIMAL 和存储的内存类型
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    // requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}

// vkCreateImage + vmaAllocateMemory + vkBindImageMemory
VkImage image;
vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);

// 创建默认 UAV view
if (desc.allowUnorderedAccess) {
    VkImageViewCreateInfo viewInfo = { ... };
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = /* 从 desc.format 转换 */;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCreateImageView(device, &viewInfo, nullptr, &defaultUAV);
}
```

#### ITexture::CreateDescriptor 确认

```cpp
// 当前 ITexture 接口
virtual uint64_t CreateDescriptor(TextureDescriptorType descType,
                                  TextureFormat format = TextureFormat::Unknown,
                                  uint32_t mipLevel = 0,
                                  uint32_t arraySize = 0) = 0;

// 当 descType == UnorderedAccessView 时，Vulkan 后端应返回
// VK_DESCRIPTOR_TYPE_STORAGE_IMAGE 对应的 VkDescriptorImageInfo
```

### Template3D 使用示例

```cpp
// 创建存储图像（替代 vmaCreateImage + vkCreateImageView）
TextureDesc storageDesc;
storageDesc.type = TextureType::Texture2D;
storageDesc.format = TextureFormat::RGBA32_Float;
storageDesc.width = width;
storageDesc.height = height;
storageDesc.allowUnorderedAccess = true;
storageDesc.allowShaderResource = true;

auto storageTexture = resourceFactory->CreateTextureImpl(storageDesc);
// 获取 UAV 描述符供描述符集绑定
uint64_t storageUAV = storageTexture->CreateDescriptor(
    TextureDescriptorType::UnorderedAccessView);
```

---

## 模块 4：Pipeline Barrier 参数化

### 问题

`ICommandBuffer::PipelineBarrier()` 无参数，Vulkan 实现为空操作。Template3D 需要图像布局转换：计算前 `UNDEFINED → GENERAL`，计算后 `GENERAL → SHADER_READ_ONLY_OPTIMAL`。

### 设计

为 `PipelineBarrier()` 增加 `ImageBarrier` 参数数组，同时引入抽象的 `ResourceState` 枚举。

```cpp
// 文件: src/engine/graphic/interfaces/ICommandBuffer.h

/// @brief 资源状态枚举（新增）
enum class ResourceState {
    Undefined,              // VK_IMAGE_LAYOUT_UNDEFINED
    Common,                 // VK_IMAGE_LAYOUT_GENERAL
    ShaderRead,             // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    UnorderedAccess,        // VK_IMAGE_LAYOUT_GENERAL (计算读写)
    CopySrc,                // VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    CopyDst,                // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    RenderTarget,           // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    DepthStencil,           // VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    Present,                // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
};

/// @brief 图像屏障描述（新增）
struct ImageBarrier {
    ITexture* texture = nullptr;
    ResourceState oldState = ResourceState::Undefined;
    ResourceState newState = ResourceState::Common;
    uint32_t mipLevel = 0;
    uint32_t arraySlice = 0;
};

// ICommandBuffer PipelineBarrier 变更
class ICommandBuffer {
public:
    // 原有无参版本保留（默认空屏障，向后兼容）
    virtual void PipelineBarrier() = 0;

    // 新增带参版本
    virtual void PipelineBarrier(const std::vector<ImageBarrier>& imageBarriers) = 0;

    // 为了方便可合并为重载签名
    // virtual void PipelineBarrier(const std::vector<ImageBarrier>& imageBarriers = {}) = 0;
};
```

### Vulkan 后端映射表

| `ResourceState` | Vulkan Layout |
|----------------|---------------|
| `Undefined` | `VK_IMAGE_LAYOUT_UNDEFINED` |
| `Common` | `VK_IMAGE_LAYOUT_GENERAL` |
| `ShaderRead` | `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` |
| `UnorderedAccess` | `VK_IMAGE_LAYOUT_GENERAL` |
| `CopySrc` | `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` |
| `CopyDst` | `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` |
| `RenderTarget` | `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` |
| `DepthStencil` | `VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL` |
| `Present` | `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` |

访问掩码和管线阶段自动推导：

| `oldState → newState` | `srcAccessMask` | `dstAccessMask` | `srcStageMask` | `dstStageMask` |
|------------------------|----------------|----------------|----------------|----------------|
| `Undefined → UnorderedAccess` | `0` | `SHADER_WRITE_BIT` | `TOP_OF_PIPE` | `COMPUTE_SHADER` |
| `UnorderedAccess → ShaderRead` | `SHADER_WRITE_BIT` | `SHADER_READ_BIT` | `COMPUTE_SHADER` | `FRAGMENT_SHADER` |

### Template3D 使用示例

```cpp
// 计算前：UNDEFINED → GENERAL
cmdBuffer->PipelineBarrier({
    { storageImage.get(), ResourceState::Undefined, ResourceState::UnorderedAccess }
});

// 调度计算...
cmdBuffer->SetComputePipeline(computePipeline);
cmdBuffer->Dispatch(...);

// 计算后：GENERAL → SHADER_READ_ONLY_OPTIMAL
cmdBuffer->PipelineBarrier({
    { storageImage.get(), ResourceState::UnorderedAccess, ResourceState::ShaderRead }
});
```

---

## Template3D 迁移映射

### 资源生命周期对照

| Vulkan 操作 | 引擎 API 替换 | 模块 |
|-------------|--------------|------|
| `vkCreateShaderModule` | `IResourceFactory::CreateShaderImpl(ShaderType::Compute, spirv)` | 已有 |
| `vmaCreateImage` + `vkCreateImageView` | `IResourceFactory::CreateTextureImpl(allowUnorderedAccess=true)` | M3 |
| `vmaCreateBuffer(UBO)` | `IResourceFactory::CreateBufferImpl(Uniform)` | 已有 |
| `vmaCreateBuffer(SSBO)` | `IResourceFactory::CreateBufferImpl(Structured, UnorderedAccess)` | 已有 |
| `vmaMapMemory` | `IBuffer::Map()` | 已有 |
| `vkCreateDescriptorSetLayout` | `IResourceFactory::CreateDescriptorSetLayout(reflection)` | 已有 |
| `vkCreatePipelineLayout` | `IComputePipeline` 内部处理 | M1 |
| `vkCreateComputePipelines` | `IComputePipeline::Create()` | M1 |
| `vkCreateDescriptorPool` | 引擎内部管理 | 已有 |
| `vkAllocateDescriptorSets` | `IResourceFactory::CreateDescriptorSet(layout)` | 已有 |
| `vkUpdateDescriptorSets` | `IDescriptorSet::BindBuffer/BindStorageImage` | M2, M3 |
| `vkCmdPipelineBarrier` | `ICommandBuffer::PipelineBarrier(ImageBarrier[])` | M4 |
| `vkCmdBindPipeline(COMPUTE)` | `ICommandBuffer::SetComputePipeline()` | M1 |
| `vkCmdBindDescriptorSets` | `ICommandBuffer::BindDescriptorSet()` | 已有 |
| `vkCmdDispatch` | `ICommandBuffer::Dispatch()` | 已有 |
| `vkDestroy*` | 引擎 RAII 管理 | 已有 |

### 数据流（每帧）

```
初始化阶段:
  resources.shader         = factory->CreateShaderImpl(ShaderType::Compute, spirv)
  resources.computePipeline = factory->CreateComputePipelineImpl()
  resources.computePipeline->SetShader(shader)
  resources.computePipeline->Create(device)
  resources.cameraUBO       = factory->CreateBufferImpl(Uniform, DynamicWrite)
  resources.sceneSSBO       = factory->CreateBufferImpl(Structured, UnorderedAccess)
  resources.storageImage    = factory->CreateTextureImpl(allowUnorderedAccess=true)
  resources.descriptorSet   = factory->CreateDescriptorSet(layout)
  resources.descriptorSet->BindBuffer(2, cameraUBO, ..., UniformBuffer)
  resources.descriptorSet->BindBuffer(3, sceneSSBO, ..., StorageBuffer)
  resources.descriptorSet->BindStorageImage(0, storageImage)

每帧渲染:
  cmdBuffer->Begin()
  cmdBuffer->PipelineBarrier({storageImage, Undefined, UnorderedAccess})
  cameraUBO->Map() → memcpy → Unmap()
  cmdBuffer->SetComputePipeline(computePipeline)
  cmdBuffer->BindDescriptorSet(0, descriptorSet)
  cmdBuffer->Dispatch(...)
  cmdBuffer->PipelineBarrier({storageImage, UnorderedAccess, ShaderRead})
  // ... present pass (已有图形管线)
  cmdBuffer->End()

销毁:
  (智能指针自动释放，引擎内部调用 vkDestroy*)
```

---

## 实现计划

### 阶段划分

| 阶段 | 内容 | 依赖 |
|------|------|------|
| **P1** | 模块 1：`IComputePipeline` 接口 + 资源工厂 + 命令缓冲区扩展 + Vulkan 实现 | 无 |
| **P2** | 模块 2：`DescriptorType` 枚举 + `BindBuffer` 扩展 + Vulkan 后端实现 | 无 |
| **P3** | 模块 3：`allowUnorderedAccess` 的 Vulkan 实现完善 + 测试 | P2（描述符集绑定存储图像） |
| **P4** | 模块 4：`ResourceState` + `ImageBarrier` + `PipelineBarrier` 参数化 + Vulkan 实现 | 无 |
| **P5** | Template3D 重构：替换全部 40+ Vulkan 调用 | P1-P4 |
| **P6** | 验证：编译、运行、比对渲染结果 | P5 |

P1-P4 可并行实现（除 P3 依赖 P2 外），P5 为最终集成。

### 文件变更清单

| 文件 | 变更类型 | 模块 |
|------|---------|------|
| `src/engine/graphic/interfaces/IComputePipeline.h` | 新建 | M1 |
| `src/engine/graphic/interfaces/IResourceFactory.h` | 修改（+`CreateComputePipelineImpl`） | M1 |
| `src/engine/graphic/interfaces/ICommandBuffer.h` | 修改（+`SetComputePipeline`, +`PipelineBarrier`重载, +`ResourceState`, +`ImageBarrier`） | M1, M4 |
| `src/engine/graphic/interfaces/IDescriptorSet.h` | 修改（+`DescriptorType`, 修改`BindBuffer`, +`BindStorageImage`） | M2 |
| `src/engine/graphic/adapters/vulkan/VulkanComputePipeline.h/.cpp` | 新建 | M1 |
| `src/engine/graphic/adapters/vulkan/VulkanResourceFactory.cpp` | 修改（+`CreateComputePipelineImpl`, 完善`CreateTextureImpl`） | M1, M3 |
| `src/engine/graphic/adapters/vulkan/VulkanCommandBuffer.cpp` | 修改（+`SetComputePipeline`, +参数化`PipelineBarrier`） | M1, M4 |
| `src/engine/graphic/adapters/vulkan/VulkanDescriptorSet.h/.cpp` | 修改（+`BufferType`转`VkDescriptorType`） | M2 |
| `projects/Template3D/src/Template3DApp.h` | 重构（删除`Vk*`成员，替换为智能指针） | P5 |
| `projects/Template3D/src/Template3DApp.cpp` | 重构（40+ API 调用全部替换） | P5 |

---

## 向后兼容性

| 接口 | 变更 | 影响 |
|------|------|------|
| `IDescriptorSet::BindBuffer` | 增加 `DescriptorType` 参数（默认值 `UniformBuffer`） | ✅ 兼容，现有调用不变 |
| `ICommandBuffer::PipelineBarrier()` | 增加重载 `PipelineBarrier(ImageBarrier[])` | ✅ 兼容，无参版本保留 |
| `IResourceFactory` | 增加 `CreateComputePipelineImpl()` | ✅ 兼容，新增纯虚函数（需所有后端实现，目前仅 Vulkan） |
| `ICommandBuffer` | 增加 `SetComputePipeline()` | ✅ 兼容，新增纯虚函数 |

> **注意**：新增纯虚函数需要所有后端（Vulkan、DX12、OpenGL）实现。已有后端需添加空实现或 `#if defined(PRISMA_ENABLE_RENDER_*)` 条件编译。

---

## 附录：Template3D 当前 Vulkan 调用汇总

| # | Vulkan 调用 | 位置（行） | 引擎 API 替换 | 优先级 |
|---|------------|-----------|--------------|--------|
| 1 | `vkCreateShaderModule` | 40 | `CreateShaderImpl` | 已有 |
| 2 | `vkCmdPipelineBarrier` | 62 | `PipelineBarrier(ImageBarrier)` | M4 |
| 3 | `vmaCreateImage` | 343 | `CreateTextureImpl` | M3 |
| 4 | `vkCreateImageView` | 358 | `CreateTextureImpl` (内部) | M3 |
| 5 | `vmaCreateBuffer` (UBO) | 374 | `CreateBufferImpl` | 已有 |
| 6 | `vmaMapMemory` (UBO) | 380 | `Map()` | 已有 |
| 7 | `vmaCreateBuffer` (SSBO) | 392 | `CreateBufferImpl` | 已有 |
| 8 | `vmaMapMemory` (SSBO) | 398 | `Map()` | 已有 |
| 9 | `vmaGetAllocationInfo` | 404 | 引擎内部 | 已有 |
| 10 | `vkFlushMappedMemoryRanges` | 410 | `Unmap()` (内部) | 已有 |
| 11 | `vkCreateDescriptorSetLayout` | 435 | `CreateDescriptorSetLayout` | 已有 |
| 12 | `vkCreatePipelineLayout` | 445 | `IComputePipeline` 内部 | M1 |
| 13 | `vkCreateComputePipelines` | 458 | `Create()` | M1 |
| 14 | `vkCreateDescriptorPool` | 480 | 引擎内部管理 | 已有 |
| 15 | `vkAllocateDescriptorSets` | 491 | `CreateDescriptorSet` | 已有 |
| 16 | `vkUpdateDescriptorSets` | 543 | `BindBuffer/BindStorageImage` | M2, M3 |
| 17-20 | present 相关 | 573-704 | (已有图形管线) | 已有 |
| 21 | `vkCmdPipelineBarrier` (compute) | 749 | `PipelineBarrier(ImageBarrier)` | M4 |
| 22 | 写入 mapping 内存 | 778 | `Map→memcpy→Unmap` | 已有 |
| 23 | `vkFlushMappedMemoryRanges` | 787 | `Unmap()` (内部) | 已有 |
| 24 | `vkCmdBindPipeline` (compute) | 790 | `SetComputePipeline` | M1 |
| 25 | `vkCmdBindDescriptorSets` | 791 | `BindDescriptorSet` | 已有 |
| 26 | `vkCmdDispatch` | 796 | `Dispatch` | 已有 |
| 27-30 | present 绘制 | 814-827 | (已有图形管线) | 已有 |
| 31-40 | 清理 `vkDestroy*`/`vmaDestroy*` | 956-1039 | RAII 自动管理 | 已有 |
