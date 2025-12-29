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

# ==============================================================================
# Android 平台渲染器 (从 PrismaAndroid 迁移)
# ==============================================================================

if(ANDROID)
    # Android Vulkan 渲染器
    set(ANDROID_VULKAN_SOURCES
        graphic/adapters/vulkan/RendererVulkan.cpp
        graphic/adapters/vulkan/ShaderVulkan.cpp
        graphic/adapters/vulkan/TextureAsset.cpp
        graphic/adapters/vulkan/VulkanContext.cpp
    )

    set(ANDROID_VULKAN_HEADERS
        graphic/adapters/vulkan/RendererAPI.h
        graphic/adapters/vulkan/RendererVulkan.h
        graphic/adapters/vulkan/ShaderVulkan.h
        graphic/adapters/vulkan/TextureAsset.h
        graphic/adapters/vulkan/VulkanContext.h
    )

    # Android OpenGL ES 渲染器
    set(ANDROID_OPENGL_SOURCES
        graphic/adapters/opengl/RendererOpenGL.cpp
        graphic/adapters/opengl/ShaderOpenGL.cpp
    )

    set(ANDROID_OPENGL_HEADERS
        graphic/adapters/opengl/RendererOpenGL.h
        graphic/adapters/opengl/ShaderOpenGL.h
    )

    # 将 Android 渲染器添加到主列表
    list(APPEND ENGINE_GRAPHIC_SOURCES ${ANDROID_VULKAN_SOURCES} ${ANDROID_OPENGL_SOURCES})
    list(APPEND ENGINE_GRAPHIC_HEADERS ${ANDROID_VULKAN_HEADERS} ${ANDROID_OPENGL_HEADERS})
endif()