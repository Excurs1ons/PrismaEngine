# 引擎核心库 - 条件编译版本

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
if(NOT PRISMA_ENABLE_AUDIO_SDL3 AND NOT PRISMA_ENABLE_RENDER_VULKAN)
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
        graphic/adapters/vulkan/RenderDeviceVulkan.cpp
        graphic/adapters/vulkan/VulkanShader.cpp
        graphic/adapters/vulkan/VulkanMaterial.cpp
        graphic/adapters/vulkan/VulkanMesh.cpp
    )
    list(APPEND ENGINE_GRAPHIC_HEADERS
        graphic/adapters/vulkan/RenderDeviceVulkan.h
        graphic/adapters/vulkan/VulkanShader.h
        graphic/adapters/vulkan/VulkanMaterial.h
        graphic/adapters/vulkan/VulkanMesh.h
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
    audio/AudioFactory.cpp
)

list(APPEND ENGINE_HEADERS
    audio/AudioTypes.h
    audio/IAudioDevice.h
    audio/AudioManager.h
    audio/AudioFactory.h
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

# ========== 创建静态库 ==========

add_library(Engine STATIC
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    pch.h
    pch.cpp
    packing/pipeline.h
)

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
    find_package(OpenAL QUIET)
    if(OpenAL_FOUND)
        target_include_directories(Engine PUBLIC ${OpenAL_INCLUDE_DIRS})
        target_link_libraries(Engine PUBLIC ${OpenAL_LIBRARIES})
    else()
        message(WARNING "OpenAL not found, but device is enabled")
    endif()
endif()

if(PRISMA_ENABLE_AUDIO_SDL3)
    # On Android, manually locate SDL3 from vcpkg
    if(ANDROID)
        # Extract the vcpkg path from CMAKE_PREFIX_PATH (first path before semicolon)
        list(GET CMAKE_PREFIX_PATH 0 VCPKG_PATH)
        message(STATUS "Searching for SDL3 in ${VCPKG_PATH}")

        # Directly set the paths since we know where they are
        set(SDL3_INCLUDE_DIRS "${VCPKG_PATH}/include/SDL3")
        set(SDL3_LIBRARIES "${VCPKG_PATH}/lib/libSDL3.a")

        # Check if files exist
        if(EXISTS "${SDL3_INCLUDE_DIRS}/SDL.h" AND EXISTS "${SDL3_LIBRARIES}")
            message(STATUS "Found SDL3: ${SDL3_LIBRARIES}")
            message(STATUS "SDL3 include dirs: ${SDL3_INCLUDE_DIRS}")
            target_include_directories(Engine PUBLIC ${SDL3_INCLUDE_DIRS})
            target_link_libraries(Engine PUBLIC ${SDL3_LIBRARIES})
        else()
            message(FATAL_ERROR "SDL3 not found in vcpkg installed directory")
            message(STATUS "VCPKG_PATH: ${VCPKG_PATH}")
            message(STATUS "SDL3_INCLUDE_DIRS: ${SDL3_INCLUDE_DIRS}")
            message(STATUS "SDL3_LIBRARIES: ${SDL3_LIBRARIES}")
        endif()
    else()
        # 强制使用静态链接的 SDL3
        set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as static library" FORCE)
        set(SDL_STATIC ON CACHE BOOL "Build SDL3 as static library" FORCE)
        find_package(SDL3 CONFIG REQUIRED)
        target_link_libraries(Engine PUBLIC SDL3::SDL3-static)
    endif()
endif()

# 渲染设备依赖
if(PRISMA_ENABLE_RENDER_VULKAN)
    if(ANDROID)
        # Android平台Vulkan是系统库
        list(GET CMAKE_PREFIX_PATH 0 VCPKG_PATH)
        set(Vulkan_INCLUDE_DIRS "${VCPKG_PATH}/include")

        # 检查Vulkan头文件是否存在
        if(EXISTS "${Vulkan_INCLUDE_DIRS}/vulkan/vulkan.h")
            message(STATUS "Found Vulkan headers for Android: ${Vulkan_INCLUDE_DIRS}/vulkan")
            target_include_directories(Engine PUBLIC ${Vulkan_INCLUDE_DIRS})

            # Android平台Vulkan是系统库，不需要静态库文件
            # 只需在链接时指定库名
            target_link_libraries(Engine PUBLIC vulkan)
        else()
            message(FATAL_ERROR "Vulkan headers not found for Android")
        endif()
    else()
        find_package(Vulkan QUIET)
        if(Vulkan_FOUND)
            target_include_directories(Engine PUBLIC ${Vulkan_INCLUDE_DIRS})
            target_link_libraries(Engine PUBLIC ${Vulkan_LIBRARIES})
        else()
            message(WARNING "Vulkan not found, but backend is enabled")
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

# 在Android平台上添加vcpkg包含路径
if(ANDROID)
    # CMAKE_PREFIX_PATH 可能是单个路径而不是列表
    set(VCPKG_PATH ${CMAKE_PREFIX_PATH})
    message(STATUS "CMAKE_PREFIX_PATH: ${VCPKG_PATH}")

    # 如果包含分号，只取第一个路径
    string(FIND "${VCPKG_PATH}" \; SEMICOLON_POS)
    if(SEMICOLON_POS GREATER -1)
        string(SUBSTRING "${VCPKG_PATH}" 0 ${SEMICOLON_POS} FIRST_PATH)
        set(VCPKG_PATH ${FIRST_PATH})
    endif()

    message(STATUS "Using VCPKG_PATH: ${VCPKG_PATH}")
    target_include_directories(Engine PUBLIC ${VCPKG_PATH}/include)

    # 添加GLM包含路径（由于vcpkg的glm没有顶层glm.hpp）
    target_include_directories(Engine PUBLIC ${VCPKG_PATH}/include/glm)

    # 查找并链接vcpkg安装的库
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
        PATHS ${VCPKG_PATH}/include
        NO_DEFAULT_PATH
    )

    # 如果上述方式失败，尝试直接从项目根目录查找
    if(NOT GLM_INCLUDE_DIR)
        set(GLM_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../vcpkg_installed/arm64-android")
        find_path(GLM_INCLUDE_DIR glm.hpp
            PATHS ${GLM_INSTALL_DIR}/include
            NO_DEFAULT_PATH
        )
        message(STATUS "Trying alternative path: ${GLM_INSTALL_DIR}")
    endif()

    if(GLM_INCLUDE_DIR)
        message(STATUS "Found GLM: ${GLM_INCLUDE_DIR}")
        target_include_directories(Engine PUBLIC ${GLM_INCLUDE_DIR})
    else()
        # 尝试绝对路径
        set(GLM_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../vcpkg_installed/arm64-android/include")
        if(EXISTS "${GLM_ABS_PATH}/glm.hpp")
            message(STATUS "Found GLM using absolute path: ${GLM_ABS_PATH}")
            target_include_directories(Engine PUBLIC ${GLM_ABS_PATH})
        else()
            message(FATAL_ERROR "GLM not found in vcpkg installed directory. Expected path: ${GLM_ABS_PATH}/glm.hpp")
        endif()
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

# Windows 平台需要链接的额外依赖
if(NOT ANDROID)
    # 查找并链接 GLM
    find_package(glm CONFIG REQUIRED)
    target_link_libraries(Engine PUBLIC glm::glm-header-only)

    # 查找并链接 nlohmann_json 和 imgui
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    target_link_libraries(Engine PUBLIC
        nlohmann_json::nlohmann_json
        imgui::imgui
    )
endif()

# 根据启用的设备定义宏
target_compile_definitions(Engine PUBLIC
    PRISMA_DEFAULT_AUDIO_DEVICE=${PRISMA_DEFAULT_AUDIO_BACKEND}
    PRISMA_DEFAULT_RENDER_DEVICE=${PRISMA_DEFAULT_RENDER_BACKEND}
)

# 安装规则
install(TARGETS Engine
    EXPORT PrismaEngineTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)