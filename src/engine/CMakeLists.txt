# 引擎核心库 - Prune 分支版本
# 仅支持 Windows x64 + Vulkan + SDL3

# 平台检测 (Windows only for build, Linux for LSP)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    add_compile_options(/DNOMINMAX /DWIN32_LEAN_AND_MEAN)
endif()

# 寻找 Vulkan
if(PRISMA_ENABLE_RENDER_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

# ========== 收集核心源文件 (所有平台共用) ==========
set(CORE_SOURCES
    # Memory
    memory/MemorySystem.cpp

    # App
    app/CommandLineParser.cpp
    app/Engine.cpp
    app/EngineLauncher.cpp
    app/EngineCAPI.cpp
    app/Application.cpp
    app/ECSApplication.cpp

    # Threading
    threading/JobSystem.cpp
    threading/ThreadManager.cpp
    threading/WorkerThread.cpp

    # Logger
    logger/Logger.cpp
    logger/LogScope.cpp

    # Object
    object/Object.cpp

    # Platform
    platform/Platform.cpp
    platform/DynamicLoader.cpp

    # Window
    window/Window.cpp

    # Scene
    scene/Scene.cpp
    scene/SceneManager.cpp

    # Serialization
    serialization/Serializable.cpp
    serialization/ScriptComponent.cpp

    # Transform
    transform/Transform.cpp
    transform/Camera.cpp
    transform/CameraController.cpp

    # Animation
    animation/AnimationSystem.cpp
    animation/Skeleton.cpp

    # Audio
    audio/SoundSynthesizer.cpp
    audio/AudioDeviceNull.cpp
    audio/AudioDeviceSDL3.cpp
    audio/AudioAPI.cpp

    # Input
    input/InputDevice.cpp
    input/InputManager.cpp

    # Console
    console/ConsoleSystem.cpp
    console/ConsoleUI.cpp

    # Physics
    physics/PhysicsSystem.cpp
    physics/RigidBody.cpp
    physics/TriggerManager.cpp

    # Navigation
    navigation/NavigationSystem.cpp

    # AI
    ai/AISystem.cpp

    # Particles
    particles/ParticleSystem.cpp

    # Terrain
    terrain/HeightMap.cpp
    terrain/TerrainMesh.cpp
    terrain/TerrainMaterial.cpp
    terrain/TerrainCollision.cpp
    terrain/TerrainSystem.cpp

    # Network
    network/TCPTransport.cpp
    network/UDPTransport.cpp
    network/Session.cpp
    network/NetworkSystem.cpp

    # Water
    water/WaterMesh.cpp
    water/WaterSystem.cpp
    water/WaveSimulation.cpp
    water/WaterInteraction.cpp

    # Localization
    localization/LocalizationFile.cpp
    localization/LocalizationManager.cpp
    localization/LocalizationSystem.cpp

    # Profiling
    profiling/ProfilerGPU.cpp
    profiling/ProfilerSystem.cpp

    # Resource
    resource/AssetSerializer.cpp
    resource/ArchiveJson.cpp
    resource/MeshAsset.cpp
    resource/ResourceFallback.cpp
    resource/ResourceFallbackImpl.cpp
    resource/TextureAsset.cpp

    # Core
    core/AssetManager.cpp
    core/AssetDatabase.cpp
    core/EntityManager.cpp
    core/Node.cpp
    core/ECS.cpp
    # core/Serialization.cpp (removed - unused)
    core/Layer.cpp
    core/LayerStack.cpp
    core/ComponentRegistry.cpp
    core/Component.cpp
    core/UUID.cpp
    core/SpriteRendererComponent.cpp
    core/Packaging.cpp

    # UI
    ui/UIComponent.cpp
    ui/UIInputManager.cpp
    ui/2d/ButtonComponent.cpp
    ui/2d/CanvasComponent.cpp

    # Math
    math/MurmurHash3.cpp

    # Utils
    utils/ImageUtils.cpp

    # Graphic
    graphic/Renderer.cpp
    graphic/Renderer2D.cpp
    graphic/RenderSystem.cpp
    graphic/RenderCommandContext.cpp
    graphic/RenderComponent.cpp
    graphic/RenderGraph.cpp
    graphic/RenderPass.cpp
    graphic/Shader.cpp
    graphic/ShaderFactory.cpp
    graphic/TextureAtlas.cpp
    graphic/VoxelRenderer.cpp
    graphic/LightComponent.cpp
    graphic/GeometryRenderPass.cpp
    graphic/Material.cpp
    graphic/Mesh.cpp
    graphic/glTFLoader.cpp
    graphic/MeshRenderer.cpp
    graphic/PrimitiveComponent.cpp
    graphic/OrthographicCamera.cpp
    graphic/PerspectiveCamera.cpp
    graphic/SpriteRenderer.cpp
    graphic/SpriteAnimation.cpp
    graphic/stb_impl.cpp
    graphic/RenderResourceManager.cpp
    graphic/SpirvReflector.cpp
    graphic/LogicalPass.cpp
    graphic/LogicalPipeline.cpp
    graphic/interfaces/IPipelineState.cpp

    # Graphic - Forward Pipeline
    graphic/pipelines/forward/ForwardPipeline.cpp
    graphic/pipelines/forward/DepthPrePass.cpp
    graphic/pipelines/forward/OpaquePass.cpp
    graphic/pipelines/forward/TransparentPass.cpp

    # Graphic - PathTracing Pipeline
    graphic/pipelines/pathtracing/PathTracingPipeline.cpp
    graphic/pipelines/pathtracing/VulkanRTBackend.cpp

    # Graphic - Deferred Pipeline
    graphic/pipelines/deferred/DeferredPipeline.cpp
    graphic/pipelines/deferred/GeometryPass.cpp
    graphic/pipelines/deferred/LightingPass.cpp
    graphic/pipelines/deferred/CompositionPass.cpp
    graphic/pipelines/deferred/GBuffer.cpp

    # Graphic - Clustered Pipeline
    graphic/pipelines/clustered/ClusteredForwardPipeline.cpp
    graphic/pipelines/clustered/ClusteredOpaquePass.cpp

    # Graphic - NPR Pipeline
    graphic/pipelines/npr/NPRPipeline.cpp
    graphic/pipelines/npr/NPROpaquePass.cpp

    # Graphic - Skybox
    graphic/pipelines/SkyboxRenderPass.cpp

    # Graphic - UI
    graphic/ui/FontAtlas.cpp
    graphic/ui/TextRendererComponent.cpp
    graphic/ui/UIPass.cpp

    # Graphic - 2D
    graphic/2d/Light2D.cpp
    graphic/2d/LightManager2D.cpp
    graphic/2d/Light2DPass.cpp
    graphic/2d/ReflectionPass2D.cpp
    graphic/2d/Graphics2D.cpp
    graphic/2d/Pipeline2D.cpp
    graphic/2d/CanvasPass2D.cpp
    graphic/2d/PostProcessPass2D.cpp
    graphic/2d/UIPass2D.cpp

    # Graphic - Vulkan Backend
    graphic/adapters/vulkan/VulkanAdapters.cpp
    graphic/adapters/vulkan/RenderDeviceVulkan.cpp
    graphic/adapters/vulkan/RenderDeviceVulkan_Capture.cpp
    graphic/adapters/vulkan/VulkanSwapChain.cpp
    graphic/adapters/vulkan/VulkanShader.cpp
    graphic/adapters/vulkan/VulkanResources.cpp
    graphic/adapters/vulkan/VulkanResourceFactory.cpp
    graphic/adapters/vulkan/VulkanFence.cpp
    graphic/adapters/vulkan/VulkanSampler.cpp
    graphic/adapters/vulkan/VulkanPipelineState.cpp
    graphic/adapters/vulkan/VulkanComputePipeline.cpp
    graphic/adapters/vulkan/VulkanCommandBuffer.cpp

    # Precompiled header
    pch.cpp
)


# ========== C# Scripting (条件编译) ==========
if(PRISMA_ENABLE_SCRIPTING STREQUAL "MONO" OR PRISMA_ENABLE_SCRIPTING STREQUAL "CORECLR")
    set(PRISMA_SCRIPTING_SOURCES
        scripting/MonoRuntime.cpp
        scripting/CoreCLRHost.cpp
        scripting/ScriptEngine.cpp
        scripting/ScriptSystem.cpp
        scripting/SRPGraphicsAPI.cpp
        scripting/EditorAPI.cpp
    )
    set(PRISMA_SCRIPTING_HEADERS
        scripting/MonoRuntime.h
        scripting/CoreCLRHost.h
        scripting/ScriptEngine.h
        scripting/ScriptSystem.h
        scripting/SRPGraphicsAPI.h
        scripting/EditorAPI.h
    )
    list(APPEND CORE_SOURCES ${PRISMA_SCRIPTING_SOURCES})

    # 构建 C# 核心库
    find_program(DOTNET dotnet)
    if(DOTNET)
        message(STATUS "Scripting: 找到 dotnet, 将构建 C# 核心库")
        
        # 动态确定运行时标识符 (RID)
        if(WIN32)
            set(CS_RID "win-x64")
        else()
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
                set(CS_RID "linux-arm64")
            else()
                set(CS_RID "linux-x64")
            endif()
        endif()

        set(CS_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripting/CSharp/Prisma.Core")
        set(CS_HOST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripting/CSharp/PrismaEngine.Host")
        set(CS_OUTPUT_FILE "${CMAKE_BINARY_DIR}/bin/Prisma.Core.dll")

        # 强制 .NET 工具链输出英文（避免中文乱码）
        set(DOTNET_ENV_CMD ${CMAKE_COMMAND} -E env
            "DOTNET_CLI_UI_LANGUAGE=en-US"
            "VSLANG=1033")

        # NuGet restore（避免并行构建冲突）
        set(CS_RESTORE_STAMP "${CMAKE_BINARY_DIR}/.prisma_core_restored")
        add_custom_command(
            OUTPUT "${CS_RESTORE_STAMP}"
            COMMAND ${DOTNET_ENV_CMD} ${DOTNET} restore "${CS_CORE_DIR}/Prisma.Core.csproj" -p:RestorePackagesWithLockFile=false
            COMMAND ${CMAKE_COMMAND} -E touch "${CS_RESTORE_STAMP}"
            DEPENDS "${CS_CORE_DIR}/Prisma.Core.csproj"
            COMMENT "Restoring Prisma.Core NuGet packages"
        )

        add_custom_command(
            OUTPUT "${CS_OUTPUT_FILE}"
            COMMAND ${DOTNET_ENV_CMD} ${DOTNET} build "${CS_CORE_DIR}/Prisma.Core.csproj" -c Release -o "${CMAKE_BINARY_DIR}/bin" --no-restore
            DEPENDS "${CS_CORE_DIR}/Prisma.Core.csproj" "${CS_RESTORE_STAMP}"
            COMMENT "Building Prisma.Core C# library"
        )

        add_custom_target(PrismaCore ALL
            DEPENDS "${CS_OUTPUT_FILE}"
        )

        # 自包含 Host 发布：产出运行时文件（hostfxr + coreclr + 所有 System DLL）
        # 发布到 build/PrismaEngine.Host/<rid>/publish/
        set(CS_HOST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/PrismaEngine.Host/${CS_RID}")

        # Host restore（避免并行冲突）
        set(CS_HOST_RESTORE_STAMP "${CMAKE_BINARY_DIR}/.prisma_host_restored")
        add_custom_command(
            OUTPUT "${CS_HOST_RESTORE_STAMP}"
            COMMAND ${DOTNET_ENV_CMD} ${DOTNET} restore "${CS_HOST_DIR}/PrismaEngine.Host.csproj" -r ${CS_RID} -p:RestorePackagesWithLockFile=false
            COMMAND ${CMAKE_COMMAND} -E touch "${CS_HOST_RESTORE_STAMP}"
            DEPENDS "${CS_HOST_DIR}/PrismaEngine.Host.csproj"
            COMMENT "Restoring PrismaEngine.Host NuGet packages"
        )

        add_custom_command(
            OUTPUT "${CS_HOST_OUTPUT_DIR}/publish/PrismaEngine.Host.runtimeconfig.json"
            COMMAND ${DOTNET_ENV_CMD} ${DOTNET} publish "${CS_HOST_DIR}/PrismaEngine.Host.csproj"
                    -c Release -r ${CS_RID} --self-contained true
                    -o "${CS_HOST_OUTPUT_DIR}/publish"
                    -p:DebugType=portable -p:DebugSymbols=true
                    --nologo --no-restore
            DEPENDS "${CS_OUTPUT_FILE}" "${CS_HOST_DIR}/PrismaEngine.Host.csproj" "${CS_HOST_RESTORE_STAMP}"
            COMMENT "Publishing PrismaEngine.Host (self-contained, RID: ${CS_RID})"
        )

        add_custom_target(PrismaEngineHost ALL
            DEPENDS "${CS_HOST_OUTPUT_DIR}/publish/PrismaEngine.Host.runtimeconfig.json"
        )

        # C# 源码生成器（Roslyn Analyzer，编译 GameScripts 时自动调用）
        set(CS_GENERATORS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripting/CSharp/Prisma.Generators")
        add_custom_target(PrismaGenerators ALL
            COMMAND ${DOTNET_ENV_CMD} ${DOTNET} build "${CS_GENERATORS_DIR}/Prisma.Generators.csproj" -c Release --nologo --no-restore
            COMMENT "Building Prisma.Generators (Roslyn source generators)"
        )
        add_dependencies(PrismaGenerators PrismaCore)
        add_dependencies(PrismaEngineHost PrismaCore)
        
    else()
        message(WARNING "Scripting: 未找到 dotnet, 无法构建 C# 核心库")
    endif()
endif()

# ========== 收集核心头文件 ==========
set(CORE_HEADERS
    # Memory
    memory/MemoryAllocator.h
    memory/PoolAllocator.h
    memory/FrameAllocator.h
    memory/StackAllocator.h
    memory/MemoryManager.h
    memory/MemorySystem.h

    # App
    app/CommandLineParser.h
    app/Engine.h
    app/ProjectConfig.h
    app/EngineLauncher.h
    app/EngineCAPI.h

    # Interfaces (纯虚接口，可被非 Engine 项目引用)
    interfaces/IEngine.h

    app/Application.h
    app/ECSApplication.h

    # Threading
    threading/JobSystem.h
    threading/ThreadManager.h
    threading/WorkerThread.h

    # Logger
    logger/Logger.h
    logger/LogScope.h
    logger/LogEntry.h
    logger/IPlatformLogger.h

    # Object
    object/Object.h
    object/Model.h
    object/MetaData.h

    # Platform
    platform/Platform.h
    platform/DynamicLoader.h

    # Window
    window/Window.h

    # Scene
    scene/Scene.h
    scene/SceneManager.h
    scene/SceneNode.h

    # Serialization
    serialization/Serializable.h
    serialization/SerializationVersion.h
    serialization/ScriptComponent.h

    # Transform
    transform/Transform.h
    transform/ITransform.h
    transform/Camera.h
    transform/CameraController.h
    transform/FPSCounter.h

    # Animation
    animation/Skeleton.h
    animation/AnimationClip.h
    animation/AnimStateMachine.h
    animation/SkinningRenderer.h
    animation/IKController.h
    animation/AnimComponent.h
    animation/AnimationSystem.h

    # Audio
    audio/SoundSynthesizer.h
    audio/AudioAPI.h
    audio/AudioDevice.h
    audio/AudioDeviceSDL3.h
    audio/AudioTypes.h
    audio/IAudioDevice.h
    audio/core/IAudioDriver.h

    # Audio DSP
    audio/dsp/AudioBuffer.h
    audio/dsp/AudioNode.h
    audio/dsp/AudioMath.h
    audio/dsp/ParameterBlock.h
    audio/dsp/AudioMemoryPool.h
    audio/dsp/SpscQueue.h
    audio/dsp/TestRenderer.h

    # Input
    input/InputDevice.h
    input/InputManager.h
    input/Input.h
    input/InputBackend.h
    input/KeyCode.h
    input/core/IInputDriver.h
    input/drivers/InputDriverSDL3.h
    input/drivers/InputDriverWin32.h

    # Console
    console/CVar.h
    console/ConsoleCommand.h
    console/ConsoleSystem.h
    console/ConsoleUI.h

    # Physics
    physics/PhysicsSystem.h
    physics/CollisionSystem.h
    physics/PhysicsComponents.h
    physics/RigidBody.h
    physics/Constraint.h
    physics/JointConstraint.h
    physics/ConstraintSolver.h
    physics/CCDSolver.h
    physics/TriggerVolume.h
    physics/TriggerManager.h

    # Navigation
    navigation/NavMesh.h
    navigation/NavMeshBuilder.h
    navigation/PathFinder.h
    navigation/PathSmoothing.h
    navigation/NavAgent.h
    navigation/NavAgentComponent.h
    navigation/NavigationSystem.h

    # AI
    ai/Blackboard.h
    ai/BehaviorTree.h
    ai/FiniteStateMachine.h
    ai/BuiltInTasks.h
    ai/AIComponent.h
    ai/AISystem.h

    # Particles
    particles/CPUParticleSystem.h
    particles/GPUParticleSystem.h
    particles/ParticleEmitterComponent.h
    particles/ParticleSystem.h

    # Terrain
    terrain/TerrainConfig.h
    terrain/HeightMap.h
    terrain/TerrainMesh.h
    terrain/TerrainMaterial.h
    terrain/TerrainCollision.h
    terrain/TerrainComponent.h
    terrain/TerrainSystem.h

    # Network
    network/NetworkTransport.h
    network/TCPTransport.h
    network/UDPTransport.h
    network/Packet.h
    network/RPCManager.h
    network/NetworkEntity.h
    network/Session.h
    network/NetworkSystem.h

    # Water
    water/WaterConfig.h
    water/WaterMesh.h
    water/WaveSimulation.h
    water/WaterInteraction.h
    water/WaterSystem.h

    # Localization
    localization/LocalizationData.h
    localization/LocalizationFile.h
    localization/LocalizationManager.h
    localization/LocalizationSystem.h
    localization/L10n.h

    # Profiling
    profiling/ProfilerCPU.h
    profiling/ProfilerGPU.h
    profiling/ProfilerMacros.h
    profiling/ProfilerSystem.h
    profiling/ProfilerPanel.h

    # Resource
    resource/AssetSerializer.h
    resource/MeshAsset.h
    resource/ResourceFallback.h
    resource/ResourceSerializer.h
    resource/TextureAsset.h
    resource/Archive.h
    resource/ArchiveBinary.h
    resource/ArchiveJson.h
    resource/DefaultShaders.h
    resource/OBJParser.h
    resource/ResourceFallbackImpl.h
    resource/embedded/EmbeddedResources.h

    # Core
    core/AssetManager.h
    core/Asset.h
    core/UUID.h
    core/ECS.h
    core/ProjectSettings.h
    # core/Serialization.h (removed - unused)
    core/Layer.h
    core/LayerStack.h
    core/Event.h
    core/Component.h
    core/ComponentRegistry.h
    graphic/LightComponent.h
    core/SpriteRendererComponent.h
    core/ISubSystem.h
    core/IWorld.h
    core/ManagerBase.h
    core/Singleton.h
    core/Handle.h
    core/StringHash.h
    core/Timestep.h
    core/IImageLoader.h
    core/ImageLoaderSTB.h
    core/HotReloadManager.h

    # UI
    ui/UIComponent.h
    ui/UIInputManager.h
    ui/2d/ButtonComponent.h
    ui/2d/CanvasComponent.h

    # Packing
    packing/Pipeline.h

    # Math
    math/MathTypes.h
    math/MatrixUtils.h
    math/MurmurHash3.h

    # Utils
    utils/ImageUtils.h

    # Config
    config/AudioBackendConfig.h
    config/EngineConfig.h
    config/RenderBackendConfig.h

    # Graphic
    graphic/interfaces/IComputePipeline.h
    graphic/pipelines/clustered/ClusteredForwardPipeline.h
    graphic/pipelines/clustered/ClusteredOpaquePass.h
    graphic/pipelines/npr/NPRPipeline.h

    # PCH
    pch.h
)

if(PRISMA_ENABLE_SCRIPTING STREQUAL "MONO" OR PRISMA_ENABLE_SCRIPTING STREQUAL "CORECLR")
    list(APPEND CORE_HEADERS ${PRISMA_SCRIPTING_HEADERS})
endif()

# ========== 创建引擎库 ==========

# Linux 下为 LSP 生成 compile_commands.json
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_library(Engine SHARED
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    pch.h
    pch.cpp
    packing/Pipeline.h
)

message(STATUS "Engine: 构建为动态库 (SHARED)")

target_compile_definitions(Engine PRIVATE
    ENGINE_EXPORTS=1
    PRISMA_ENGINE_EXPORTS=1
    PrismaEngine_EXPORTS=1
)
if(MSVC)
    # 强制包含 intrin.h 以确保 __faststorefence 被正确识别为内建函数
    target_compile_options(Engine PRIVATE /FIintrin.h)
endif()
target_compile_definitions(Engine PUBLIC ENGINE_BUILD_SHARED=1)

# 渲染后端宏定义 (仅 Vulkan)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_RENDER_VULKAN=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_SDL3=1)
target_compile_definitions(Engine PUBLIC PRISMA_PROFILING_ENABLED=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_DSP=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_MINIAUDIO=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_SYNTHESIZER=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_EFFECTS=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_RAYTRACING=0)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_CODEC_OGG=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_CODEC_MP3=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_CODEC_FLAC=1)

# C# Scripting 编译定义
#   PRISMA_ENABLE_SCRIPTING=0 → OFF（不编译脚本子系统）
#   PRISMA_ENABLE_SCRIPTING=1 → MONO（Mono 运行时）
#   PRISMA_ENABLE_SCRIPTING=2 → CORECLR（.NET CoreCLR 宿主）
if(PRISMA_ENABLE_SCRIPTING STREQUAL "MONO")
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_SCRIPTING=1)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_MONO=1)
    message(STATUS "Scripting: Mono 运行时")
elseif(PRISMA_ENABLE_SCRIPTING STREQUAL "CORECLR")
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_SCRIPTING=2)
    message(STATUS "Scripting: .NET CoreCLR 运行时")
else()
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_SCRIPTING=0)
    message(STATUS "Scripting: 关闭")
endif()

set_target_properties(Engine PROPERTIES OUTPUT_NAME Prisma)

# 确保 Engine 构建完成后，将 Prisma.Core.dll 复制到 Engine 的实际输出目录 (Debug/Release)
if(PRISMA_ENABLE_SCRIPTING STREQUAL "MONO" OR PRISMA_ENABLE_SCRIPTING STREQUAL "CORECLR")
    if(DOTNET)
        add_custom_command(TARGET Engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CS_OUTPUT_FILE}"
                "$<TARGET_FILE_DIR:Engine>/Prisma.Core.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/bin/Prisma.Core.pdb"
                "$<TARGET_FILE_DIR:Engine>/Prisma.Core.pdb"
            COMMENT "Deploying Prisma.Core.dll to Engine output directory"
        )
        add_dependencies(Engine PrismaCore PrismaEngineHost)

        # 编译时嵌入 Host 发布目录路径，运行时无需猜测 CWD
        target_compile_definitions(Engine PRIVATE
            PRISMA_HOST_DIR="${CS_HOST_OUTPUT_DIR}/publish"
        )
    endif()
endif()
if(MSVC)
    target_compile_definitions(Engine PRIVATE _USE_MATH_DEFINES)
    target_compile_options(Engine PRIVATE 
        /utf-8
        $<$<CONFIG:Release>:/O2 /Oi /Gy>
        $<$<CONFIG:MinSizeRel>:/O1 /Oi /Gy>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Gy>
    )
    # Release 也生成 PDB 符号文件（用于崩溃转储分析）
    target_compile_options(Engine PRIVATE $<$<CONFIG:Release>:/Zi>)
    target_link_options(Engine PRIVATE $<$<CONFIG:Release>:/DEBUG>)
    # SPIRV-Tools 预编译库不提供 PDB，压制无害的 LNK4099
    target_link_options(Engine PRIVATE /ignore:4099)
    # 压制第三方库（glaze、VMA、STL实例化等）的内部警告
    target_compile_options(Engine PRIVATE /wd4702 /wd4324 /wd4251 /wd4100 /wd4189 /wd4244)
endif()

# ========== 链接依赖项 ==========
target_link_libraries(Engine
    PUBLIC
        glm::glm-header-only
        glaze::glaze
        stb
        cgltf::cgltf
    PRIVATE
        imgui::imgui
)

if(PRISMA_ENABLE_AUDIO_SDL3)
    # SDL3 库链接
    # 此处使用 PUBLIC 链接，因为 Engine 的公有头文件 (如 Window.h) 和 
    # 外部项目 (如 PathTracing3D) 需要引用 SDL3 的头文件。
    if(TARGET SDL3-shared)
        target_link_libraries(Engine PUBLIC SDL3-shared)
    elseif(TARGET SDL3::SDL3-shared)
        target_link_libraries(Engine PUBLIC SDL3::SDL3-shared)
    elseif(TARGET SDL3-static)
        target_link_libraries(Engine PUBLIC SDL3-static)
    elseif(TARGET SDL3)
        target_link_libraries(Engine PUBLIC SDL3)
    endif()
endif()

if(PRISMA_ENABLE_AUDIO_MINIAUDIO)
    target_link_libraries(Engine PUBLIC miniaudio)
endif()

if(PRISMA_ENABLE_RENDER_VULKAN)
    target_link_libraries(Engine PUBLIC Vulkan::Vulkan)
    if(TARGET vma)
        target_link_libraries(Engine PUBLIC vma)
    elseif(TARGET VulkanMemoryAllocator)
        target_link_libraries(Engine PUBLIC VulkanMemoryAllocator)
    endif()
    if(TARGET vk-bootstrap::vk-bootstrap)
        target_link_libraries(Engine PUBLIC vk-bootstrap::vk-bootstrap)
    endif()
    if(TARGET spirv-reflect-static)
        target_link_libraries(Engine PUBLIC spirv-reflect-static)
    elseif(TARGET spirv-reflect)
        target_link_libraries(Engine PUBLIC spirv-reflect)
    endif()
    # glslang — GLSL→SPIR-V 编译
    if(TARGET glslang::glslang)
        target_link_libraries(Engine PRIVATE
            glslang::glslang
            glslang::SPIRV
            glslang::OSDependent
            glslang::MachineIndependent
            glslang::GenericCodeGen
            glslang::glslang-default-resource-limits
            SPIRV-Tools
            SPIRV-Tools-opt
        )
        target_compile_definitions(Engine PRIVATE PRISMA_HAS_GLSLANG=1)
        message(STATUS "  Engine links glslang: GLSL→SPIR-V enabled")
    else()
        message(STATUS "  Engine: GLSL→SPIR-V disabled (glslang not found)")
    endif()
endif()

# ========== 平台链接 (Windows WinSock2) ==========
if(WIN32)
    target_link_libraries(Engine PUBLIC ws2_32)
endif()

# ========== 粒子系统着色器编译 ==========
set(PARTICLES_SHADER_DIR "${PROJECT_SOURCE_DIR}/resources/common/shaders/glsl/particles")
file(GLOB PARTICLES_SHADER_SOURCES
    "${PARTICLES_SHADER_DIR}/*.vert"
    "${PARTICLES_SHADER_DIR}/*.frag"
    "${PARTICLES_SHADER_DIR}/*.comp"
)
if(PARTICLES_SHADER_SOURCES)
    find_program(GLSLANG_VALIDATOR glslangValidator)
    if(GLSLANG_VALIDATOR)
        foreach(SHADER ${PARTICLES_SHADER_SOURCES})
            get_filename_component(SHADER_NAME ${SHADER} NAME)
            set(SPV_OUTPUT "${PARTICLES_SHADER_DIR}/${SHADER_NAME}.spv")
            add_custom_command(
                OUTPUT ${SPV_OUTPUT}
                COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${SPV_OUTPUT}
                DEPENDS ${SHADER}
                COMMENT "编译粒子着色器: ${SHADER_NAME} → SPIR-V"
            )
            list(APPEND PARTICLES_GENERATED_SPVS ${SPV_OUTPUT})
        endforeach()
        add_custom_target(ParticleShaders DEPENDS ${PARTICLES_GENERATED_SPVS})
        add_dependencies(Engine ParticleShaders)
        message(STATUS "粒子着色器 SPIR-V 编译已启用 (glslangValidator: ${GLSLANG_VALIDATOR})")
    else()
        message(WARNING "glslangValidator 未找到 — 粒子着色器将跳过 SPIR-V 编译，使用已有的 .spv")
    endif()
endif()

# ========== 水体着色器编译 ==========
set(WATER_SHADER_DIR "${PROJECT_SOURCE_DIR}/resources/common/shaders/glsl/water")
file(GLOB WATER_SHADER_SOURCES
    "${WATER_SHADER_DIR}/*.vert"
    "${WATER_SHADER_DIR}/*.frag"
    "${WATER_SHADER_DIR}/*.comp"
)
if(WATER_SHADER_SOURCES)
    find_program(GLSLANG_VALIDATOR glslangValidator)
    if(GLSLANG_VALIDATOR)
        foreach(SHADER ${WATER_SHADER_SOURCES})
            get_filename_component(SHADER_NAME ${SHADER} NAME)
            set(SPV_OUTPUT "${WATER_SHADER_DIR}/${SHADER_NAME}.spv")
            add_custom_command(
                OUTPUT ${SPV_OUTPUT}
                COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${SPV_OUTPUT}
                DEPENDS ${SHADER}
                COMMENT "Compiling water shader: ${SHADER_NAME} -> SPIR-V"
            )
            list(APPEND WATER_GENERATED_SPVS ${SPV_OUTPUT})
        endforeach()
        add_custom_target(WaterShaders DEPENDS ${WATER_GENERATED_SPVS})
        add_dependencies(Engine WaterShaders)
        message(STATUS "Water shader SPIR-V compilation enabled (glslangValidator: ${GLSLANG_VALIDATOR})")
    else()
        message(WARNING "glslangValidator not found - water shaders will skip SPIR-V compilation, using existing .spv")
    endif()
endif()

target_include_directories(Engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/math>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resource>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/audio>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/audio/dsp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/graphic>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/2d>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/input>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/memory>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/app>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/scene>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/object>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/transform>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/threading>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/serialization>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/logger>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/window>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platform>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/physics>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/particles>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/terrain>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/navigation>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/network>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/water>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ai>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/localization>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/animation>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/scripting>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/packing>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
)

target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/glm-src")
target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/stb-src")
target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/vma-src/include")
target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/dr_libs-src")

# Linux 平台额外 include
if(LINUX)
    target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/zstd-src/lib")
endif()


