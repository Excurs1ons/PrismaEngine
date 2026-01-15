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
    graphic/RenderAPIVulkan.cpp
)

set(VULKAN_HEADERS
    graphic/RenderAPIVulkan.h
)

set(ENGINE_GRAPHIC_SOURCES
    ${PIPELINE_FORWARD_SOURCES}
    ${PIPELINE_DEFERRED_SOURCES}
    # ${UI_SOURCES}  # 暂时禁用：UIPass.cpp 实现被注释掉了
    ${LOGICAL_SOURCES}
    graphic/pipelines/SkyboxRenderPass.cpp
    graphic/GeometryRenderPass.cpp
    graphic/Material.cpp
    graphic/Mesh.cpp
    graphic/MeshRenderer.cpp
    graphic/RenderComponent.cpp
    graphic/RenderCommandContext.cpp
    graphic/RenderPass.cpp
    graphic/RenderSystemNew.cpp
    # graphic/ScriptableRenderPipeline.cpp  # 已删除：使用 LogicalPipeline 替代
    graphic/Shader.cpp
    graphic/stb_impl.cpp
    # New interface files
    graphic/interfaces/RenderTypes.cpp
    graphic/interfaces/IPipelineState.cpp
    # DX12 adapter files (conditionally included)
    # graphic/adapters/dx12/ files will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_DX12
    # Vulkan backend files (conditionally included)
    # VULKAN_SOURCES will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_VULKAN
)

set(ENGINE_GRAPHIC_HEADERS
    ${PIPELINE_FORWARD_HEADERS}
    ${PIPELINE_DEFERRED_HEADERS}
    # ${UI_HEADERS}  # 暂时禁用：UIPass.cpp 实现被注释掉了
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
    graphic/RenderSystemNew.h
    # graphic/ScriptableRenderPipeline.h  # 已删除：使用 LogicalPipeline 替代
    graphic/ICamera.h
    graphic/Shader.h
    # New interface headers
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
    # New logic layer interfaces
    graphic/interfaces/IDeviceContext.h
    graphic/interfaces/IRenderTarget.h
    graphic/interfaces/IPass.h
    graphic/interfaces/IGBuffer.h
    # DX12 adapter headers (conditionally included)
    # graphic/adapters/dx12/ files will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_DX12
    # Vulkan backend headers (conditionally included)
    # VULKAN_HEADERS will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_VULKAN
)

# Android 平台渲染器保留在 PrismaAndroid 子模块中
# 不在这里重复编译

# ========== 超分辨率模块 / Upscaler Module ==========

# 超分辨率核心文件（总是需要）
set(UPSCALER_CORE_SOURCES
    graphic/upscaler/UpscalerManager.cpp
    graphic/upscaler/UpscalerPass.cpp
)

set(UPSCALER_CORE_HEADERS
    graphic/upscaler/UpscalerManager.h
    graphic/upscaler/UpscalerPass.h
    graphic/interfaces/IUpscaler.h
)

# 运动矢量 Pass（如果启用）
if(PRISMA_ENABLE_UPSCALER_MOTION_VECTORS)
    list(APPEND UPSCALER_CORE_SOURCES
        graphic/upscaler/MotionVectorPass.cpp
    )
    list(APPEND UPSCALER_CORE_HEADERS
        graphic/upscaler/MotionVectorPass.h
    )
endif()

# FSR 实现
if(PRISMA_ENABLE_UPSCALER_FSR)
    list(APPEND UPSCALER_CORE_SOURCES
        graphic/upscaler/adapters/FSR/UpscalerFSR.cpp
        graphic/upscaler/adapters/FSR/FSRResources.cpp
    )
    list(APPEND UPSCALER_CORE_HEADERS
        graphic/upscaler/adapters/FSR/UpscalerFSR.h
        graphic/upscaler/adapters/FSR/FSRResources.h
    )
    message(STATUS "  Upscaler: FSR enabled")
endif()

# DLSS 实现
if(PRISMA_ENABLE_UPSCALER_DLSS)
    # TODO: 添加 DLSS 源文件
    message(STATUS "  Upscaler: DLSS enabled (sources to be added)")
endif()

# TSR 实现
if(PRISMA_ENABLE_UPSCALER_TSR)
    # TODO: 添加 TSR 源文件
    message(STATUS "  Upscaler: TSR enabled (sources to be added)")
endif()

# 添加到引擎图形库
list(APPEND ENGINE_GRAPHIC_SOURCES ${UPSCALER_CORE_SOURCES})
list(APPEND ENGINE_GRAPHIC_HEADERS ${UPSCALER_CORE_HEADERS})