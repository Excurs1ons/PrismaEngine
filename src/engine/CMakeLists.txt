# 引擎核心库 - 条件编译版本

# ========== 库类型配置 ==========

option(PRISMA_ENGINE_BUILD_SHARED "Build engine as shared library (DLL/so)" OFF)

# 设置库类型变量
if(PRISMA_ENGINE_BUILD_SHARED)
    set(PRISMA_ENGINE_TYPE SHARED)
    set(PRISMA_ENGINE_LIBRARY_TYPE SHARED)
    message(STATUS "Engine: Building as SHARED library")
else()
    set(PRISMA_ENGINE_TYPE STATIC)
    set(PRISMA_ENGINE_LIBRARY_TYPE STATIC)
    message(STATUS "Engine: Building as STATIC library")
endif()

# 包含graphic子目录的CMakeLists.txt来定义变量
include(graphic/CMakeLists.txt)

# 收集源文件
set(ENGINE_SOURCES "")
set(ENGINE_HEADERS "")

# 添加core目录下的源文件（除了graphic和audio目录）
file(GLOB_RECURSE CORE_SOURCES
    "*.cpp"
)
list(FILTER CORE_SOURCES EXCLUDE REGEX "graphic/")
list(FILTER CORE_SOURCES EXCLUDE REGEX "audio/")

# 排除Windows特定文件
if(ANDROID)
    list(FILTER CORE_SOURCES EXCLUDE REGEX "framework.h")
    list(FILTER CORE_SOURCES EXCLUDE REGEX "targetver.h")
endif()

# 如果未启用SDL3，排除SDL相关文件
if(NOT PRISMA_ENABLE_AUDIO_SDL3)
    list(FILTER CORE_SOURCES EXCLUDE REGEX "PlatformSDL")
    message(STATUS "SDL3 is disabled, excluding PlatformSDL files")
endif()

file(GLOB_RECURSE CORE_HEADERS
    "*.h")

list(FILTER CORE_HEADERS EXCLUDE REGEX "graphic/")
list(FILTER CORE_HEADERS EXCLUDE REGEX "audio/")

# 排除Windows特定头文件
if(ANDROID)
    list(FILTER CORE_HEADERS EXCLUDE REGEX "framework.h")
    list(FILTER CORE_HEADERS EXCLUDE REGEX "targetver.h")
endif()

# 在非Android平台上排除Android特定头文件
if(NOT ANDROID)
    list(FILTER CORE_HEADERS EXCLUDE REGEX "PlatformAndroid.h")
endif()

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

# XAudio2 (Windows)
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    list(APPEND ENGINE_SOURCES
        audio/AudioDeviceXAudio2.cpp
    )
    list(APPEND ENGINE_HEADERS
        audio/AudioDeviceXAudio2.h
    )
    message(STATUS "Adding XAudio2 audio device")
endif()

# OpenAL (跨平台)
if(PRISMA_ENABLE_AUDIO_OPENAL)
    list(APPEND ENGINE_SOURCES
        audio/AudioDeviceOpenAL.cpp
    )
    list(APPEND ENGINE_HEADERS
        audio/AudioDeviceOpenAL.h
    )
    message(STATUS "Adding OpenAL audio device")
endif()

# SDL3 Audio (跨平台)
if(PRISMA_ENABLE_AUDIO_SDL3)
    list(APPEND ENGINE_SOURCES
        audio/AudioDeviceSDL3.cpp
    )
    list(APPEND ENGINE_HEADERS
        audio/AudioDeviceSDL3.h
    )
    message(STATUS "Adding SDL3 audio device")
endif()

# ========== 创建引擎库 ==========

add_library(Engine ${PRISMA_ENGINE_TYPE}
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    pch.h
    pch.cpp
    packing/pipeline.h
)

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

if(PRISMA_ENABLE_AUDIO_OPENAL)
    # OpenAL 由 FetchContent.cmake 管理
    # 尝试所有可能的 OpenAL 目标名称
    get_cmake_property(_TARGETS TARGETS)
    message(STATUS "Available targets containing 'OpenAL' or 'openal':")
    foreach(_TARGET ${_TARGETS})
        if(_TARGET MATCHES "[Oo]pen[Aa][Ll]")
            message(STATUS "  - ${_TARGET}")
        endif()
    endforeach()

    if(TARGET OpenAL::OpenAL)
        target_link_libraries(Engine PUBLIC OpenAL::OpenAL)
        message(STATUS "Linking OpenAL::OpenAL")
    elseif(TARGET OpenAL)
        target_link_libraries(Engine PUBLIC OpenAL)
        message(STATUS "Linking OpenAL")
    elseif(TARGET openal-soft)
        target_link_libraries(Engine PUBLIC openal-soft)
        message(STATUS "Linking openal-soft")
    else()
        # 回退到传统的 find_package
        find_package(OpenAL QUIET)
        if(OpenAL_FOUND)
            target_include_directories(Engine PUBLIC ${OpenAL_INCLUDE_DIRS})
            target_link_libraries(Engine PUBLIC ${OpenAL_LIBRARIES})
            message(STATUS "Linking OpenAL via find_package")
        else()
            message(WARNING "OpenAL not found, but device is enabled")
        endif()
    endif()
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

# ========== 设置包含目录 ==========

target_include_directories(Engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resource>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/audio>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/graphic>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
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

# Windows 平台额外依赖
if(WIN32 AND NOT ANDROID)
    # DirectX-Headers (PRIVATE - 内部使用)
    if(TARGET Microsoft::DirectX-Headers)
        target_link_libraries(Engine PRIVATE Microsoft::DirectX-Headers)
    elseif(TARGET DirectX-Headers)
        target_link_libraries(Engine PRIVATE DirectX-Headers)
    endif()

    # ImGui - 编辑器 UI 框架 (PUBLIC - Editor 需要使用)
    if(TARGET imgui::imgui)
        target_link_libraries(Engine PUBLIC imgui::imgui)
    elseif(TARGET imgui)
        target_link_libraries(Engine PUBLIC imgui)
    endif()
endif()

# 根据启用的设备定义宏
target_compile_definitions(Engine PUBLIC
    PRISMA_DEFAULT_AUDIO_DEVICE=${PRISMA_DEFAULT_AUDIO_BACKEND}
    PRISMA_DEFAULT_RENDER_DEVICE=${PRISMA_DEFAULT_RENDER_BACKEND}
)

# 定义音频后端启用宏（用于 AudioAPI.cpp 中的条件编译）
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_XAUDIO2=1)
endif()

if(PRISMA_ENABLE_AUDIO_OPENAL)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_OPENAL=1)
endif()

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

if(PRISMA_ENABLE_RENDER_METAL)
    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_RENDER_METAL=1)
endif()

# 安装规则
install(TARGETS Engine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)