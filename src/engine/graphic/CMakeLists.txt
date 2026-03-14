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

# 基础图形源文件
set(ENGINE_GRAPHIC_SOURCES
    graphic/RenderSystem.cpp
    graphic/RenderCommandContext.cpp
    graphic/RenderComponent.cpp
    graphic/RenderGraph.cpp
    graphic/RenderPass.cpp
    graphic/Shader.cpp
    graphic/ShaderFactory.cpp
    graphic/TextureAtlas.cpp
    graphic/VoxelRenderer.cpp
    graphic/EngineShaderAdapter.cpp
    graphic/GeometryRenderPass.cpp
    graphic/Material.cpp
    graphic/Mesh.cpp
    graphic/MeshRenderer.cpp
    graphic/stb_impl.cpp
    graphic/RenderResourceManager.cpp
    graphic/RenderAPIVulkan.cpp
    ${PIPELINE_FORWARD_SOURCES}
    ${PIPELINE_DEFERRED_SOURCES}
    ${UI_SOURCES}
    ${LOGICAL_SOURCES}
)

set(ENGINE_GRAPHIC_HEADERS
    graphic/RenderSystem.h
    graphic/RenderCommandContext.h
    graphic/RenderComponent.h
    graphic/RenderGraph.h
    graphic/RenderGraphCore.h
    graphic/RenderPass.h
    graphic/Shader.h
    graphic/ShaderFactory.h
    graphic/TextureAtlas.h
    graphic/VoxelRenderer.h
    graphic/EngineShaderAdapter.h
    graphic/GeometryRenderPass.h
    graphic/Material.h
    graphic/Mesh.h
    graphic/MeshRenderer.h
    graphic/DefaultShader.h
    graphic/FrustumCulling.h
    graphic/ICamera.h
    graphic/RenderDesc.h
    graphic/RenderErrorHandling.h
    graphic/RenderException.h
    graphic/SubMesh.h
    graphic/test_shader.h
    graphic/RenderResourceManager.h
    graphic/RenderAPIVulkan.h
    ${PIPELINE_FORWARD_HEADERS}
    ${PIPELINE_DEFERRED_HEADERS}
    ${UI_HEADERS}
    ${LOGICAL_HEADERS}
)


# Vulkan 后端文件 (条件编译)
set(VULKAN_SOURCES
    graphic/adapters/vulkan/VulkanAdapters.cpp
    graphic/adapters/vulkan/RenderDeviceVulkan.cpp
    graphic/adapters/vulkan/VulkanShader.cpp
    graphic/adapters/vulkan/VulkanResources.cpp
    graphic/adapters/vulkan/VulkanResourceFactory.cpp
    graphic/adapters/vulkan/VulkanFence.cpp
    graphic/adapters/vulkan/VulkanSampler.cpp
    graphic/adapters/vulkan/VulkanPipelineState.cpp
)

set(VULKAN_HEADERS
    graphic/adapters/vulkan/VulkanAdapters.h
    graphic/adapters/vulkan/RenderDeviceVulkan.h
    graphic/adapters/vulkan/VulkanResourceFactory.h
    graphic/adapters/vulkan/VulkanSwapChain.h
    graphic/adapters/vulkan/VulkanShader.h
    graphic/adapters/vulkan/VulkanMaterial.h
    graphic/adapters/vulkan/VulkanMesh.h
    graphic/adapters/vulkan/VulkanResources.h
    graphic/adapters/vulkan/VulkanFence.h
    graphic/adapters/vulkan/VulkanSampler.h
    graphic/adapters/vulkan/VulkanPipelineState.h
)

# ========== 渲染后端条件编译 / Render Backend Conditional Compilation ==========

if(PRISMA_ENABLE_RENDER_VULKAN)
    list(APPEND ENGINE_GRAPHIC_SOURCES ${VULKAN_SOURCES})
    list(APPEND ENGINE_GRAPHIC_HEADERS ${VULKAN_HEADERS})
endif()