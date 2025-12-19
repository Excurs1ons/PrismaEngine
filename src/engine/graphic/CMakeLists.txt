set(PIPELINE_FORWARD_SOURCES
    graphic/pipelines/forward/ForwardPipeline.cpp
    graphic/pipelines/forward/ForwardRenderPass.cpp
    graphic/pipelines/forward/DepthPrePass.cpp
    graphic/pipelines/forward/OpaquePass.cpp
    graphic/pipelines/forward/TransparentPass.cpp
)

set(PIPELINE_FORWARD_HEADERS
    graphic/pipelines/forward/ForwardPipeline.h
    graphic/pipelines/forward/ForwardRenderPass.h
    graphic/pipelines/forward/DepthPrePass.h
    graphic/pipelines/forward/OpaquePass.h
    graphic/pipelines/forward/TransparentPass.h
)

set(ENGINE_GRAPHIC_SOURCES
    ${PIPELINE_FORWARD_SOURCES}
    graphic/pipelines/SkyboxRenderPass.cpp
    graphic/GeometryRenderPass.cpp
    graphic/Material.cpp
    graphic/Mesh.cpp
    graphic/MeshRenderer.cpp
    graphic/RenderBackendDirectX12.cpp
    graphic/RenderBackendVulkan.cpp
    graphic/RenderComponent.cpp
    graphic/RenderPass.cpp

    graphic/ScriptableRenderPipeline.cpp
    graphic/Shader.cpp
    # New interface files
    graphic/interfaces/RenderTypes.cpp
    # New adapter files
    graphic/adapters/dx12/DX12RenderDevice.cpp
    graphic/adapters/dx12/DX12CommandBuffer.cpp
    graphic/adapters/dx12/DX12Fence.cpp
    graphic/adapters/dx12/DX12SwapChain.cpp
    graphic/adapters/dx12/DX12Texture.cpp
    graphic/adapters/dx12/DX12Buffer.cpp
    graphic/adapters/dx12/DX12Shader.cpp
    graphic/adapters/dx12/DX12PipelineState.cpp
    graphic/adapters/dx12/DX12Sampler.cpp
    graphic/adapters/dx12/DX12ResourceFactory.cpp
    # Test files
    graphic/RenderSystemTest.cpp
)

set(ENGINE_GRAPHIC_HEADERS
    ${PIPELINE_FORWARD_HEADERS}
    graphic/pipelines/SkyboxRenderPass.h
    graphic/GeometryRenderPass.h
    graphic/Material.h
    graphic/Mesh.h
    graphic/MeshRenderer.h
    graphic/RenderBackend.h
    graphic/RenderBackendDirectX12.h
    graphic/RenderBackendVulkan.h
    graphic/RenderCommandContext.h
    graphic/RenderComponent.h
    graphic/RenderGraph.h
    graphic/RenderPass.h
    graphic/RenderSystem.h

    graphic/ScriptableRenderPipeline.h
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
    # New adapter headers
    graphic/adapters/dx12/DX12Adapters.h
    graphic/adapters/dx12/DX12RenderDevice.h
    graphic/adapters/dx12/DX12CommandBuffer.h
    graphic/adapters/dx12/DX12Fence.h
    graphic/adapters/dx12/DX12SwapChain.h
    graphic/adapters/dx12/DX12Texture.h
    graphic/adapters/dx12/DX12Buffer.h
    graphic/adapters/dx12/DX12Shader.h
    graphic/adapters/dx12/DX12PipelineState.h
    graphic/adapters/dx12/DX12Sampler.h
    graphic/adapters/dx12/DX12ResourceFactory.h
)