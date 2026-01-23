# 引擎核心库 - 条件编译版本

# Windows平台定义 - 必须在任何include之前
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    add_compile_options(/DNOMINMAX /DWIN32_LEAN_AND_MEAN)
endif()

# 包含graphic子目录的CMakeLists.txt来定义变量
include(graphic/CMakeLists.txt)

# ========== 收集核心源文件 (所有平台共用) ==========
# 核心基础文件
set(CORE_SOURCES
    CommandLineParser.cpp
    Common.cpp
    Engine.cpp
    JobSystem.cpp
    Logger.cpp
    LogScope.cpp
    Object.cpp
    PhysicsSystem.cpp
    Platform.cpp
    PlatformSDL.cpp
    ResourceManagerNew.cpp
    Scene.cpp
    SceneManager.cpp
    Serializable.cpp
    ThreadManager.cpp
    WorkerThread.cpp
    # 场景对象
    Camera.cpp
    CameraController.cpp
    GameObject.cpp
    ScriptComponent.cpp
    Transform.cpp
    # 资源
    CubemapTextureAsset.cpp
    TextureAsset.cpp
    # 音效合成器
    SoundSynthesizer.cpp
    # 示例/测试
    TriangleExample.cpp
    # 预编译头
    pch.cpp
    # 脚本系统
    scripting/MonoRuntime.cpp
    scripting/ScriptSystem.cpp
    # ECS 系统
    core/AssetManager.cpp
    core/ECS.cpp
    # UI 系统
    ui/UIComponent.cpp
    ui/UIInputManager.cpp
    ui/2d/ButtonComponent.cpp
    ui/2d/CanvasComponent.cpp
    # 音频系统（暂时全部禁用，需要重写以匹配新系统）
    # audio/AudioAPI.cpp
    # audio/AudioDevice.cpp
    audio/AudioDeviceNull.cpp
    # audio/AudioManager.cpp
)

# ========== 条件编译音频后端（新系统） ==========
# SDL3 音频后端
if(PRISMA_ENABLE_AUDIO_SDL3)
    list(APPEND CORE_SOURCES
        audio/AudioDeviceSDL3.cpp
    )
endif()

# XAudio2 音频后端（暂时禁用，需要修复接口）
# if(PRISMA_ENABLE_AUDIO_XAUDIO2)
#     list(APPEND CORE_SOURCES
#         audio/AudioDeviceXAudio2.cpp
#         audio/drivers/AudioDriverXAudio2.cpp
#     )
# endif()

# ========== 基础源文件（继续）==========
list(APPEND CORE_SOURCES
    # 输入系统
    input/InputDevice.cpp
    input/InputManager.cpp
    InputManager.cpp
    # 资源系统
    resource/Asset.cpp
    resource/AssetSerializer.cpp
    resource/MeshAsset.cpp
    resource/ResourceFallback.cpp
    resource/ResourceFallbackImpl.cpp
    resource/TextureAsset.cpp
    # 瓦片地图系统
    tilemap/TilemapAsset.cpp
    tilemap/format/TileDataDecoder.cpp
    tilemap/format/TmxParser.cpp
    tilemap/format/TsxParser.cpp
    tilemap/renderer/TilemapRenderer.cpp
)

# ========== 平台特定源文件 (条件编译) ==========
# Windows 平台
if(WIN32)
    list(APPEND CORE_SOURCES
        PlatformWindows.cpp
        platform/Application.cpp
        platform/ApplicationWindows.cpp
        platform/WindowsUtils.cpp
        input/drivers/InputDriverWin32.cpp
    )
endif()

# Linux 平台
if(LINUX)
    list(APPEND CORE_SOURCES
        PlatformLinux.cpp
        platform/ApplicationLinux.cpp
    )
endif()

# Android 平台
if(ANDROID)
    list(APPEND CORE_SOURCES
        PlatformAndroid.cpp
        AndroidLogger.cpp
        AndroidOut.cpp
    )
endif()

# ========== 收集核心头文件 ==========
set(CORE_HEADERS
    CommandLineParser.h
    Common.h
    Engine.h
    JobSystem.h
    Logger.h
    LogScope.h
    Object.h
    PhysicsSystem.h
    Platform.h
    ResourceManagerNew.h
    Scene.h
    SceneManager.h
    Serializable.h
    ThreadManager.h
    WorkerThread.h
    # 场景对象
    Camera.h
    CameraController.h
    GameObject.h
    ScriptComponent.h
    Transform.h
    # 资源
    CubemapTextureAsset.h
    TextureAsset.h
    # 音效合成器
    SoundSynthesizer.h
    # 示例/测试
    TriangleExample.h
    # 预编译头
    pch.h
    packing/Pipeline.h
    # 脚本系统
    scripting/MonoRuntime.h
    scripting/ScriptSystem.h
    # ECS 系统
    core/AssetManager.h
    core/ECS.h
    # UI 系统
    ui/UIComponent.h
    ui/UIInputManager.h
    ui/2d/ButtonComponent.h
    ui/2d/CanvasComponent.h
    # 音频系统（新系统）
    audio/AudioAPI.h
    audio/AudioDevice.h
    audio/AudioManager.h
    audio/AudioTypes.h
    audio/IAudioDevice.h
    # 输入系统
    input/InputDevice.h
    input/InputManager.h
    # 资源系统
    resource/Asset.h
    resource/AssetSerializer.h
    resource/MeshAsset.h
    resource/ResourceFallback.h
    resource/TextureAsset.h
    resource/TilemapAsset.h
    # 瓦片地图系统
    tilemap/TilemapAsset.h
    tilemap/core/Types.h
    tilemap/core/Tile.h
    tilemap/core/Tileset.h
    tilemap/core/TileLayer.h
    tilemap/core/ObjectLayer.h
    tilemap/core/ImageLayer.h
    tilemap/core/GroupLayer.h
    tilemap/core/Map.h
    tilemap/format/TmxParser.h
    tilemap/format/TsxParser.h
    tilemap/format/TileDataDecoder.h
    tilemap/renderer/TilemapRenderer.h
    tilemap/renderer/TileGeometryBuilder.h
    tilemap/renderer/TileChunk.h
    tilemap/renderer/AnimatedTileManager.h
)

# 平台特定头文件
if(WIN32)
    list(APPEND CORE_HEADERS
        platform/Application.h
        platform/WindowsUtils.h
    )
endif()

# Debug 模式下添加编辑器相关
if(PRISMA_ENABLE_IMGUI_DEBUG)
    list(APPEND CORE_SOURCES
        DebugOverlay.cpp
        EditorApplication.cpp
    )
    list(APPEND CORE_HEADERS
        DebugOverlay.h
        Build.h
        EditorApplication.h
    )
endif()

message(STATUS "Collected ${CORE_SOURCES} core source files")
message(STATUS "Collected ${CORE_HEADERS} core header files")

# ========== 渲染设备条件编译 ==========
# 注意：必须在设置 ENGINE_SOURCES 之前添加，因为 ENGINE_SOURCES 使用 ENGINE_GRAPHIC_SOURCES 的值

# DirectX12 (Windows)
if(PRISMA_ENABLE_RENDER_DX12)
    # 添加所有DX12适配器文件
    list(APPEND ENGINE_GRAPHIC_SOURCES
        graphic/adapters/dx12/DX12Adapters.cpp
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
    )
    list(APPEND ENGINE_GRAPHIC_HEADERS
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
        graphic/adapters/dx12/DX12Material.h
        graphic/adapters/dx12/DX12Mesh.h
    )
    message(STATUS "Adding DirectX12 render device")
endif()

# OpenGL (跨平台)
if(PRISMA_ENABLE_RENDER_OPENGL)
    list(APPEND ENGINE_GRAPHIC_SOURCES
        graphic/adapters/opengl/RenderDeviceOpenGL.cpp
    )
    list(APPEND ENGINE_GRAPHIC_HEADERS
        graphic/adapters/opengl/RenderDeviceOpenGL.h
    )
    message(STATUS "Adding OpenGL render device")
endif()

# Vulkan (跨平台)
if(PRISMA_ENABLE_RENDER_VULKAN)
    list(APPEND ENGINE_GRAPHIC_SOURCES
        graphic/adapters/vulkan/VulkanShader.cpp
        ${VULKAN_SOURCES}
    )
    list(APPEND ENGINE_GRAPHIC_HEADERS
        graphic/adapters/vulkan/VulkanShader.h
        ${VULKAN_HEADERS}
    )
    message(STATUS "Adding Vulkan render device")
endif()

# 添加基础核心文件
set(ENGINE_SOURCES
    ${CORE_SOURCES}
    ${ENGINE_GRAPHIC_SOURCES}
)

# Debug 模式下添加 ImGui 支持
if(PRISMA_ENABLE_IMGUI_DEBUG)
    list(APPEND ENGINE_SOURCES DebugOverlay.cpp)
    list(APPEND ENGINE_HEADERS DebugOverlay.h Build.h)
    message(STATUS "Adding ImGui debug support")
endif()

set(ENGINE_HEADERS
    ${CORE_HEADERS}
    ${ENGINE_GRAPHIC_HEADERS}
)

# ========== 音频设备条件编译 ==========

# 基础音频文件（总是包含）
list(APPEND ENGINE_SOURCES
    audio/AudioAPI.cpp
)

list(APPEND ENGINE_HEADERS
    audio/AudioTypes.h
    audio/IAudioDevice.h
    audio/AudioManager.h
    audio/AudioAPI.h
)

# ========== 创建引擎库 ==========

# 根据 PRISMA_BUILD_SHARED_LIBS 选项选择库类型
if(PRISMA_BUILD_SHARED_LIBS)
    add_library(Engine SHARED
        ${ENGINE_SOURCES}
        ${ENGINE_HEADERS}
        pch.h
        pch.cpp
        packing/Pipeline.h
    )
    message(STATUS "Engine: 构建为动态库 (SHARED)")
    target_compile_definitions(Engine PRIVATE ENGINE_BUILD_SHARED=1)
else()
    add_library(Engine STATIC
        ${ENGINE_SOURCES}
        ${ENGINE_HEADERS}
        pch.h
        pch.cpp
        packing/Pipeline.h
    )
    message(STATUS "Engine: 构建为静态库 (STATIC)")
    target_compile_definitions(Engine PRIVATE ENGINE_BUILD_STATIC=1)
endif()

# Android 平台添加 game-activity 源文件
if(ANDROID)
    target_sources(Engine PRIVATE
        ${GAME_ACTIVITY_SOURCES}
    )
    target_include_directories(Engine PRIVATE
            ${GAME_ACTIVITY_INCLUDE_DIRS}
    )
    target_link_libraries(Engine PUBLIC
        ${GAME_ACTIVITY_LIB}
        android
        log
    )
endif()

# ========== 导出符号配置 ==========

# 动态库需要导出宏，静态库不需要
if(PRISMA_ENGINE_BUILD_SHARED)
    if(WIN32)
        # Windows DLL 导出/导入宏
        target_compile_definitions(Engine PRIVATE
            ENGINE_BUILD_SHARED=1
            ENGINE_EXPORTS=1
        )
    else()
        # Linux/Unix 共享库
        target_compile_definitions(Engine PRIVATE
            ENGINE_BUILD_SHARED=1
        )
    endif()
else()
    # 静态库不需要导出宏
    target_compile_definitions(Engine PRIVATE
        ENGINE_BUILD_STATIC=1
    )
endif()

# Windows 平台添加 UTF-8 编译支持
if(WIN32)
    target_compile_options(Engine PUBLIC /utf-8)
endif()

# ========== 依赖项配置 ==========

# 音频设备依赖
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    if(WIN32)
        target_link_libraries(Engine PUBLIC xaudio2.lib)
    endif()
endif()

# XInput 游戏手柄支持 (Windows)
if(WIN32)
    target_link_libraries(Engine PUBLIC xinput9_1_0.lib)
    message(STATUS "添加 XInput 库链接")
endif()


if(PRISMA_ENABLE_AUDIO_SDL3)
    # SDL3 由 FetchContent.cmake 管理
    # 尝试所有可能的 SDL3 目标名称
    get_cmake_property(_TARGETS TARGETS)
    message(STATUS "Available targets containing 'SDL':")
    foreach(_TARGET ${_TARGETS})
        if(_TARGET MATCHES "SDL")
            message(STATUS "  - ${_TARGET}")
        endif()
    endforeach()

    if(TARGET SDL3-static)
        target_link_libraries(Engine PUBLIC SDL3-static)
        message(STATUS "Linking SDL3-static")
    elseif(TARGET SDL3::SDL3-static)
        target_link_libraries(Engine PUBLIC SDL3::SDL3-static)
        message(STATUS "Linking SDL3::SDL3-static")
    elseif(TARGET SDL3_static)
        target_link_libraries(Engine PUBLIC SDL3_static)
        message(STATUS "Linking SDL3_static")
    elseif(TARGET SDL3::SDL3-shared)
        target_link_libraries(Engine PUBLIC SDL3::SDL3-shared)
        message(STATUS "Linking SDL3::SDL3-shared")
    elseif(TARGET SDL3_shared)
        target_link_libraries(Engine PUBLIC SDL3_shared)
        message(STATUS "Linking SDL3_shared")
    elseif(TARGET SDL3)
        target_link_libraries(Engine PUBLIC SDL3)
        message(STATUS "Linking SDL3")
    elseif(TARGET SDL3::SDL3)
        target_link_libraries(Engine PUBLIC SDL3::SDL3)
        message(STATUS "Linking SDL3::SDL3")
    else()
        message(FATAL_ERROR "SDL3 target not found. Checked: SDL3-static, SDL3::SDL3-static, SDL3_static, SDL3::SDL3-shared, SDL3_shared, SDL3, SDL3::SDL3")
    endif()
endif()

# 渲染设备依赖
if(PRISMA_ENABLE_RENDER_VULKAN)
    if(ANDROID)
        # Android 平台 Vulkan 是系统库
        target_link_libraries(Engine PUBLIC vulkan)
        # Android 平台需要 game-activity_static
        target_link_libraries(Engine PUBLIC game-activity_static)
    else()
        # Windows 平台: 使用 FetchContent 提供的 Vulkan-Headers
        # 注意: Vulkan-Headers 只提供头文件，运行时需要 Vulkan SDK 或驱动
        if(TARGET Vulkan::Headers)
            target_link_libraries(Engine PUBLIC Vulkan::Headers)
        elseif(TARGET Vulkan-Headers)
            target_link_libraries(Engine PUBLIC Vulkan-Headers)
        else()
            # 回退到系统查找
            find_package(Vulkan QUIET)
            if(Vulkan_FOUND)
                target_include_directories(Engine PUBLIC ${Vulkan_INCLUDE_DIRS})
                target_link_libraries(Engine PUBLIC ${Vulkan_LIBRARIES})
            else()
                message(WARNING "Vulkan headers not found, but backend is enabled")
            endif()
        endif()

        # 链接 VMA (Vulkan Memory Allocator)
        if(TARGET GPUOpen::VulkanMemoryAllocator)
            target_link_libraries(Engine PUBLIC GPUOpen::VulkanMemoryAllocator)
            message(STATUS "Linking GPUOpen::VulkanMemoryAllocator")
        elseif(TARGET vma)
            target_link_libraries(Engine PUBLIC vma)
            message(STATUS "Linking vma")
        else()
            message(WARNING "VMA target not found. Tried: GPUOpen::VulkanMemoryAllocator, vma")
        endif()

        # 链接 vk-bootstrap
        if(TARGET vk-bootstrap::vk-bootstrap)
            target_link_libraries(Engine PUBLIC vk-bootstrap::vk-bootstrap)
            message(STATUS "Linking vk-bootstrap::vk-bootstrap")
        elseif(TARGET vk-bootstrap)
            target_link_libraries(Engine PUBLIC vk-bootstrap)
            message(STATUS "Linking vk-bootstrap")
        else()
            message(WARNING "vk-bootstrap target not found. Tried: vk-bootstrap::vk-bootstrap, vk-bootstrap")
        endif()
    endif()
endif()

if(PRISMA_ENABLE_RENDER_OPENGL)
    find_package(OpenGL QUIET)
    if(OPENGL_FOUND)
        target_include_directories(Engine PUBLIC ${OPENGL_INCLUDE_DIRS})
        target_link_libraries(Engine PUBLIC ${OPENGL_LIBRARIES})
    else()
        message(WARNING "OpenGL not found, but backend is enabled")
    endif()

    find_package(glad QUIET)
    if(glad_FOUND)
        target_include_directories(Engine PUBLIC ${glad_INCLUDE_DIRS})
        target_link_libraries(Engine PUBLIC ${glad_LIBRARIES})
    endif()

    find_package(glfw3 QUIET)
    if(GLFW3_FOUND)
        target_include_directories(Engine PUBLIC ${glfw3_INCLUDE_DIRS})
        target_link_libraries(Engine PUBLIC glfw)
    endif()
endif()

# DirectX12 (Windows) - 需要 DirectX-Headers
if(PRISMA_ENABLE_RENDER_DX12)
    if(WIN32)
        # 链接 DirectX-Headers
        if(TARGET Microsoft::DirectX-Headers)
            target_link_libraries(Engine PUBLIC Microsoft::DirectX-Headers)
            message(STATUS "链接 Microsoft::DirectX-Headers (DX12 渲染设备)")
        elseif(TARGET DirectX-Headers)
            target_link_libraries(Engine PUBLIC DirectX-Headers)
            message(STATUS "链接 DirectX-Headers (DX12 渄染设备)")
        else()
            message(FATAL_ERROR "DirectX-Headers 目标未找到! DX12 渲染设备需要 DirectX-Headers")
        endif()

        # 添加 DirectX 12 运行时库链接
        target_link_libraries(Engine PUBLIC
            d3d12.lib
            d3dcompiler.lib
            dxgi.lib
            dxguid.lib
        )
        message(STATUS "添加 DirectX 12 运行时库链接")
    else()
        message(FATAL_ERROR "DirectX12 仅支持 Windows 平台")
    endif()
endif()

# ========== 设置包含目录 ==========

target_include_directories(Engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/math>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resource>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/audio>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/graphic>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/input>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tilemap>
)

# Android平台依赖路径配置
if(ANDROID)
    # Android 平台优先使用 FetchContent 下载的依赖
    # 依赖会被下载到 ${CMAKE_BINARY_DIR}/_deps/

    # 如果需要使用外部依赖路径，可通过 MY_DEPS_PATH 传入
    if(MY_DEPS_PATH)
        set(DEPS_PATH ${MY_DEPS_PATH})
        message(STATUS "Android using injected DEPS_PATH: ${DEPS_PATH}")
        target_include_directories(Engine PUBLIC ${DEPS_PATH})
        target_include_directories(Engine PUBLIC ${DEPS_PATH}/glm)
    endif()
endif()
# ========== 编译定义 ==========

# 根据选项选择数学库
if(WIN32 AND PRISMA_USE_DIRECTXMATH AND NOT PRISMA_FORCE_GLM)
    # DirectX-Headers (PRIVATE - 内部使用)
    if(TARGET Microsoft::DirectX-Headers)
        target_link_libraries(Engine PRIVATE Microsoft::DirectX-Headers)
        message(STATUS "链接 Microsoft::DirectX-Headers 到 Engine")
        # 调试信息
        get_target_property(DX_HEADERS_INCLUDE Microsoft::DirectX-Headers INTERFACE_INCLUDE_DIRECTORIES)
        message(STATUS "  Microsoft::DirectX-Headers INTERFACE_INCLUDE_DIRECTORIES: ${DX_HEADERS_INCLUDE}")
    elseif(TARGET DirectX-Headers)
        target_link_libraries(Engine PRIVATE DirectX-Headers)
        message(STATUS "链接 DirectX-Headers 到 Engine")
        # 调试信息
        get_target_property(DX_HEADERS_INCLUDE DirectX-Headers INTERFACE_INCLUDE_DIRECTORIES)
        message(STATUS "  DirectX-Headers INTERFACE_INCLUDE_DIRECTORIES: ${DX_HEADERS_INCLUDE}")
    else()
        message(WARNING "DirectX-Headers 目标未找到!")
    endif()
    # Windows平台使用DirectXMath
    target_compile_definitions(Engine PUBLIC
        PRISMA_USE_DIRECTXMATH
        PRISMA_USING_DIRECTXMATH=1
    )
    message(STATUS "Using DirectXMath on Windows")
else()
    # 默认使用GLM
    target_compile_definitions(Engine PUBLIC
        PRISMA_USE_GLM
        PRISMA_USING_GLM=1
    )
    message(STATUS "Using GLM")
endif()


# 基础依赖 (GLM, nlohmann_json, stb)
# 注意: 这些依赖由主 CMakeLists.txt 中的 FetchContent.cmake 管理

# 链接 GLM (使用 target 链接，但由于 GLM 是 header-only，不会出现 install(EXPORT) 问题)
if(TARGET glm::glm-header-only)
    target_link_libraries(Engine PUBLIC glm::glm-header-only)
elseif(TARGET glm)
    target_link_libraries(Engine PUBLIC glm)
endif()

# 链接 nlohmann_json (PUBLIC - Editor 等模块需要使用)
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(Engine PUBLIC nlohmann_json::nlohmann_json)
elseif(TARGET nlohmann_json)
    target_link_libraries(Engine PUBLIC nlohmann_json)
endif()

# 添加 stb include 目录 (header-only 库)
if(TARGET stb)
    # stb target created by FetchContent
    target_link_libraries(Engine PUBLIC stb)
elseif(DEFINED stb_SOURCE_DIR)
    # 手动添加 stb 源目录到 include 路径
    target_include_directories(Engine PRIVATE ${stb_SOURCE_DIR})
endif()

# ImGui - 当启用 ImGui debug 或编辑器时链接
if(PRISMA_ENABLE_IMGUI_DEBUG OR PRISMA_BUILD_EDITOR)
    if(TARGET imgui::imgui)
        target_link_libraries(Engine PUBLIC imgui::imgui)
        message(STATUS "链接 imgui::imgui 到 Engine")
    elseif(TARGET imgui)
        target_link_libraries(Engine PUBLIC imgui)
        message(STATUS "链接 imgui 到 Engine")
    else()
        message(WARNING "ImGui 目标未找到!")
    endif()
endif()

# 瓦片地图系统依赖
# tinyxml2 - XML 解析 (TMX 格式)
if(TARGET tinyxml2::tinyxml2)
    target_link_libraries(Engine PUBLIC tinyxml2::tinyxml2)
    message(STATUS "链接 tinyxml2::tinyxml2 到 Engine")
elseif(TARGET tinyxml2)
    target_link_libraries(Engine PUBLIC tinyxml2)
    message(STATUS "链接 tinyxml2 到 Engine")
elseif(TARGET tinyxml2static)
    target_link_libraries(Engine PUBLIC tinyxml2static)
    message(STATUS "链接 tinyxml2static 到 Engine")
else()
    message(WARNING "tinyxml2 目标未找到!")
endif()

# Zstandard - 压缩库 (Base64+zstd 格式)
if(TARGET libzstd_shared)
    target_link_libraries(Engine PUBLIC libzstd_shared)
    message(STATUS "链接 libzstd_shared 到 Engine")
elseif(TARGET zstd::libzstd_shared)
    target_link_libraries(Engine PUBLIC zstd::libzstd_shared)
    message(STATUS "链接 zstd::libzstd_shared 到 Engine")
elseif(TARGET zstd)
    target_link_libraries(Engine PUBLIC zstd)
    message(STATUS "链接 zstd 到 Engine")
else()
    message(WARNING "zstd 目标未找到!")
endif()

# 根据启用的设备定义宏
target_compile_definitions(Engine PUBLIC
    PRISMA_DEFAULT_AUDIO_DEVICE=${PRISMA_DEFAULT_AUDIO_BACKEND}
    PRISMA_DEFAULT_RENDER_DEVICE=${PRISMA_DEFAULT_RENDER_BACKEND}
)

# 定义音频后端启用宏（用于 AudioAPI.cpp 中的条件编译）
# XAudio2 暂时禁用，需要修复接口
# if(PRISMA_ENABLE_AUDIO_XAUDIO2)
#     target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_XAUDIO2=1)
# endif()

if(PRISMA_ENABLE_AUDIO_SDL3)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_SDL3=1)
endif()

# 定义渲染后端启用宏（用于 Shader.cpp 等文件中的条件编译）
if(PRISMA_ENABLE_RENDER_DX12)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_RENDER_DX12=1)
endif()

if(PRISMA_ENABLE_RENDER_OPENGL)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_RENDER_OPENGL=1)
endif()

if(PRISMA_ENABLE_RENDER_VULKAN)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_RENDER_VULKAN=1)
endif()
