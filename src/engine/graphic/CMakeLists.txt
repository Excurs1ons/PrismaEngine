# 前向渲染管线源文件
set(PIPELINE_FORWARD_SOURCES
    graphic/pipelines/forward/ForwardPipeline.cpp
    graphic/pipelines/forward/DepthPrePass.cpp
    graphic/pipelines/forward/OpaquePass.cpp
    graphic/pipelines/forward/TransparentPass.cpp
)

# 延迟渲染管线源文件
set(PIPELINE_DEFERRED_SOURCES
    graphic/pipelines/deferred/DeferredPipeline.cpp
    graphic/pipelines/deferred/GeometryPass.cpp
    graphic/pipelines/deferred/LightingPass.cpp
    graphic/pipelines/deferred/CompositionPass.cpp
    graphic/pipelines/deferred/GBuffer.cpp
)

# UI 模块源文件
set(UI_SOURCES
    graphic/ui/FontAtlas.cpp
    graphic/ui/TextRendererComponent.cpp
    graphic/ui/UIPass.cpp
)

# UI 模块头文件
set(UI_HEADERS
    graphic/ui/FontAtlas.h
    graphic/ui/TextRendererComponent.h
    graphic/ui/UIPass.h
)

# 新增逻辑 Pass/Pipeline 源文件
set(LOGICAL_SOURCES
    graphic/LogicalPass.cpp
    graphic/LogicalPipeline.cpp
)

# 新增逻辑 Pass/Pipeline 头文件
set(LOGICAL_HEADERS
    graphic/LogicalPass.h
    graphic/LogicalPipeline.h
)

# 前向渲染管线头文件
set(PIPELINE_FORWARD_HEADERS
    graphic/pipelines/forward/ForwardPipeline.h
    graphic/pipelines/forward/DepthPrePass.h
    graphic/pipelines/forward/OpaquePass.h
    graphic/pipelines/forward/TransparentPass.h
)

# 延迟渲染管线头文件
set(PIPELINE_DEFERRED_HEADERS
    graphic/pipelines/deferred/DeferredPipeline.h
    graphic/pipelines/deferred/GeometryPass.h
    graphic/pipelines/deferred/LightingPass.h
    graphic/pipelines/deferred/CompositionPass.h
    graphic/pipelines/deferred/GBuffer.h
)

# Vulkan 后端文件 (条件编译)
set(VULKAN_SOURCES
    graphic/adapters/vulkan/VulkanAdapters.cpp
    graphic/adapters/vulkan/RenderDeviceVulkan.cpp
    graphic/adapters/vulkan/VulkanShader.cpp
)

set(VULKAN_HEADERS
    graphic/adapters/vulkan/VulkanAdapters.h
    graphic/adapters/vulkan/RenderDeviceVulkan.h
    graphic/adapters/vulkan/VulkanResourceFactory.h
    graphic/adapters/vulkan/VulkanSwapChain.h
    graphic/adapters/vulkan/VulkanShader.h
    graphic/adapters/vulkan/VulkanMaterial.h
    graphic/adapters/vulkan/VulkanMesh.h
)

set(ENGINE_GRAPHIC_SOURCES
    ${PIPELINE_FORWARD_SOURCES}
    ${PIPELINE_DEFERRED_SOURCES}
    ${UI_SOURCES}
    ${LOGICAL_SOURCES}
    graphic/pipelines/SkyboxRenderPass.cpp
    graphic/GeometryRenderPass.cpp
    graphic/Material.cpp
    graphic/Mesh.cpp
    graphic/MeshRenderer.cpp
    graphic/RenderComponent.cpp
    graphic/RenderCommandContext.cpp
    graphic/RenderPass.cpp
    graphic/RenderSystem.cpp
    graphic/Shader.cpp
    graphic/stb_impl.cpp
    graphic/TextureAtlas.cpp
    graphic/VoxelRenderer.cpp
    graphic/interfaces/RenderTypes.cpp
    graphic/interfaces/IPipelineState.cpp
)

set(ENGINE_GRAPHIC_HEADERS
    ${PIPELINE_FORWARD_HEADERS}
    ${PIPELINE_DEFERRED_HEADERS}
    ${UI_HEADERS}
    ${LOGICAL_HEADERS}
    graphic/pipelines/SkyboxRenderPass.h
    graphic/GeometryRenderPass.h
    graphic/Material.h
    graphic/Mesh.h
    graphic/MeshRenderer.h
    graphic/RenderCommandContext.h
    graphic/RenderComponent.h
    graphic/RenderGraph.h
    graphic/RenderPass.h
    graphic/RenderSystem.h
    graphic/ICamera.h
    graphic/Shader.h
    graphic/TextureAtlas.h
    graphic/VoxelRenderer.h
    graphic/interfaces/RenderTypes.h
    graphic/interfaces/Interfaces.h
    graphic/interfaces/IResource.h
    graphic/interfaces/IRenderDevice.h
    graphic/interfaces/ICommandBuffer.h
    graphic/interfaces/IResourceManager.h
    graphic/interfaces/IResourceFactory.h
    graphic/interfaces/ITexture.h
    graphic/interfaces/IBuffer.h
    graphic/interfaces/IShader.h
    graphic/interfaces/IPipelineState.h
    graphic/interfaces/IPipeline.h
    graphic/interfaces/ISampler.h
    graphic/interfaces/ISwapChain.h
    graphic/interfaces/IFence.h
    graphic/interfaces/IDeviceContext.h
    graphic/interfaces/IRenderTarget.h
    graphic/interfaces/IPass.h
    graphic/interfaces/IGBuffer.h
)

# ========== 超分辨率模块 / Upscaler Module ==========

set(UPSCALER_CORE_SOURCES
    graphic/upscaler/UpscalerManager.cpp
    graphic/upscaler/UpscalerPass.cpp
)

set(UPSCALER_CORE_HEADERS
    graphic/upscaler/UpscalerManager.h
    graphic/upscaler/UpscalerPass.h
    graphic/interfaces/IUpscaler.h
)

if(PRISMA_ENABLE_UPSCALER_MOTION_VECTORS)
    list(APPEND UPSCALER_CORE_SOURCES graphic/upscaler/MotionVectorPass.cpp)
    list(APPEND UPSCALER_CORE_HEADERS graphic/upscaler/MotionVectorPass.h)
endif()

if(PRISMA_ENABLE_UPSCALER_FSR)
    list(APPEND UPSCALER_CORE_SOURCES
        graphic/upscaler/adapters/FSR/UpscalerFSR.cpp
        graphic/upscaler/adapters/FSR/FSRResources.cpp
    )
    list(APPEND UPSCALER_CORE_HEADERS
        graphic/upscaler/adapters/FSR/UpscalerFSR.h
        graphic/upscaler/adapters/FSR/FSRResources.h
    )
endif()

if(PRISMA_ENABLE_UPSCALER_DLSS)
    list(APPEND UPSCALER_CORE_SOURCES
        graphic/upscaler/adapters/DLSS/UpscalerDLSS.cpp
        graphic/upscaler/adapters/DLSS/DLSSResources.cpp
    )
    list(APPEND UPSCALER_CORE_HEADERS
        graphic/upscaler/adapters/DLSS/UpscalerDLSS.h
        graphic/upscaler/adapters/DLSS/DLSSResources.h
    )
endif()

if(PRISMA_ENABLE_UPSCALER_TSR)
    list(APPEND UPSCALER_CORE_SOURCES graphic/upscaler/adapters/TSR/UpscalerTSR.cpp)
    list(APPEND UPSCALER_CORE_HEADERS graphic/upscaler/adapters/TSR/UpscalerTSR.h)
endif()

list(APPEND ENGINE_GRAPHIC_SOURCES ${UPSCALER_CORE_SOURCES})
list(APPEND ENGINE_GRAPHIC_HEADERS ${UPSCALER_CORE_HEADERS})
