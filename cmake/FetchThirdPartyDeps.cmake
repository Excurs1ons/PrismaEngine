# FetchContent.cmake
# 使用 CMake FetchContent 替代 vcpkg 管理依赖

# 包含 CMake 内置的 FetchContent 模块
# 让 CMake 自动查找内置模块，避免 CMAKE_ROOT 路径问题
include(FetchContent)

# 包含版本锁定配置
include(${CMAKE_CURRENT_LIST_DIR}/DependencyVersions.cmake OPTIONAL)

# ========== 选项配置 ==========

option(PRISMA_USE_FETCHCONTENT "使用 FetchContent 下载依赖 (否则使用系统/已安装库)" ON)
option(PRISMA_FETCHCONTENT_UPDATE "更新 FetchContent 依赖 (DISCARD_ON更新)" OFF)
option(PRISMA_FETCHCONTENT_QUIET "安静模式 (减少输出)" ON)

# 更新策略
if(PRISMA_FETCHCONTENT_UPDATE)
    set(FETCHCONTENT_UPDATES_DISCONNECTED OFF)
else()
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
endif()

# ========== 依赖版本配置 ==========

# GLM - 数学库
set(FETCHCONTENT_GLM_DIR "${FETCHCONTENT_BASE_DIR}/glm")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG ${PRISMA_DEP_GLM_VERSION}
    GIT_SHALLOW TRUE
)

# nlohmann/json - JSON库
set(FETCHCONTENT_NLOHMANN_JSON_DIR "${FETCHCONTENT_BASE_DIR}/nlohmann_json")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG ${PRISMA_DEP_NLOHMANN_JSON_VERSION}
    GIT_SHALLOW FALSE
)

# SDL3 - 窗口/输入/音频抽象
set(FETCHCONTENT_SDL3_DIR "${FETCHCONTENT_BASE_DIR}/sdl3")
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG ${PRISMA_DEP_SDL3_VERSION}
    GIT_SHALLOW FALSE
)

# Vulkan-Headers - Vulkan头文件
set(FETCHCONTENT_VULKAN_HEADERS_DIR "${FETCHCONTENT_BASE_DIR}/vulkan-headers")
FetchContent_Declare(
    Vulkan-Headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG ${PRISMA_DEP_VULKAN_HEADERS_VERSION}
    GIT_SHALLOW FALSE
)

# VMA (Vulkan Memory Allocator) - Vulkan内存分配库
set(FETCHCONTENT_VMA_DIR "${FETCHCONTENT_BASE_DIR}/vma")
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG ${PRISMA_DEP_VMA_VERSION}
    GIT_SHALLOW FALSE
)

# vk-bootstrap - Vulkan初始化库
set(FETCHCONTENT_VK_BOOTSTRAP_DIR "${FETCHCONTENT_BASE_DIR}/vk-bootstrap")
FetchContent_Declare(
    vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarge/vk-bootstrap.git
    GIT_TAG ${PRISMA_DEP_VK_BOOTSTRAP_VERSION}
    GIT_SHALLOW FALSE
)

# DirectX-Headers (Windows) - DirectX 12 头文件
if(WIN32)
    set(FETCHCONTENT_DIRECTX_HEADERS_DIR "${FETCHCONTENT_BASE_DIR}/directx-headers")
    FetchContent_Declare(
        DirectX-Headers
        GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
        GIT_TAG ${PRISMA_DEP_DIRECTX_HEADERS_VERSION}
        GIT_SHALLOW FALSE
    )
endif()

# ImGui - UI框架 (带docking分支)
set(FETCHCONTENT_IMGUI_DIR "${FETCHCONTENT_BASE_DIR}/imgui")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG ${PRISMA_DEP_IMGUI_VERSION}
    GIT_SHALLOW FALSE
)

# OpenFBX - FBX模型加载
if(WIN32)
    set(FETCHCONTENT_OPENFBX_DIR "${FETCHCONTENT_BASE_DIR}/openfbx")
    FetchContent_Declare(
        openfbx
        GIT_REPOSITORY https://github.com/nem0/OpenFBX.git
        GIT_TAG ${PRISMA_DEP_OPENFBX_VERSION}
        GIT_SHALLOW FALSE
    )
endif()

# stb - 图像加载库 (header-only)
set(FETCHCONTENT_STB_DIR "${FETCHCONTENT_BASE_DIR}/stb")
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG ${PRISMA_DEP_STB_VERSION}
    GIT_SHALLOW FALSE
)

# Tweeny - 补间动画库 (header-only, 用于 UI 动画)
set(FETCHCONTENT_TWEENY_DIR "${FETCHCONTENT_BASE_DIR}/tweeny")
FetchContent_Declare(
    tweeny
    GIT_REPOSITORY https://github.com/mobius3/tweeny.git
    GIT_TAG ${PRISMA_DEP_TWEENY_VERSION}
    GIT_SHALLOW FALSE
)

# tinyxml2 - XML解析库 (用于 TMX 地图格式)
set(FETCHCONTENT_TINYXML2_DIR "${FETCHCONTENT_BASE_DIR}/tinyxml2")
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG ${PRISMA_DEP_TINYXML2_VERSION}
    GIT_SHALLOW FALSE
)

# Zstandard - 压缩库 (用于 TMX Base64+zstd 格式)
set(FETCHCONTENT_ZSTD_DIR "${FETCHCONTENT_BASE_DIR}/zstd")
FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG ${PRISMA_DEP_ZSTD_VERSION}
    GIT_SHALLOW FALSE
)

# ========== 基础库 (总是需要) ==========

message(STATUS "")
message(STATUS "=== FetchContent 依赖管理 ===")
message(STATUS "")
message(STATUS "依赖版本锁定 / Dependency Versions:")
message(STATUS "  GLM:              ${PRISMA_DEP_GLM_VERSION}")
message(STATUS "  nlohmann_json:    ${PRISMA_DEP_NLOHMANN_JSON_VERSION}")
message(STATUS "  stb:              ${PRISMA_DEP_STB_VERSION}")
message(STATUS "  tinyxml2:         ${PRISMA_DEP_TINYXML2_VERSION}")
message(STATUS "  zstd:             ${PRISMA_DEP_ZSTD_VERSION}")
message(STATUS "  Tweeny:           ${PRISMA_DEP_TWEENY_VERSION}")
message(STATUS "  SDL3:             ${PRISMA_DEP_SDL3_VERSION}")
message(STATUS "  Vulkan-Headers:   ${PRISMA_DEP_VULKAN_HEADERS_VERSION}")
message(STATUS "  VMA:              ${PRISMA_DEP_VMA_VERSION}")
message(STATUS "  vk-bootstrap:     ${PRISMA_DEP_VK_BOOTSTRAP_VERSION}")
if(WIN32)
    message(STATUS "  DirectX-Headers:  ${PRISMA_DEP_DIRECTX_HEADERS_VERSION}")
    message(STATUS "  ImGui:            ${PRISMA_DEP_IMGUI_VERSION}")
    message(STATUS "  OpenFBX:          ${PRISMA_DEP_OPENFBX_VERSION}")
endif()
message(STATUS "")
message(STATUS "更新策略 / Update Strategy:")
if(PRISMA_FETCHCONTENT_UPDATE)
    message(STATUS "  允许更新依赖 / Updates: ENABLED")
    message(WARNING "  警告: 依赖版本可能会发生变化!")
else()
    message(STATUS "  锁定当前版本 / Updates: DISABLED (FETCHCONTENT_UPDATES_DISCONNECTED=ON)")
endif()
message(STATUS "")
message(STATUS "最后验证 / Last Verified: ${PRISMA_DEP_LAST_VERIFIED}")
message(STATUS "==============================")
message(STATUS "")

# GLM (总是需要)
# 禁用 GLM 的测试和示例
set(GLM_BUILD_TESTS OFF CACHE BOOL "Build GLM tests" FORCE)
set(GLM_BUILD_EXAMPLES OFF CACHE BOOL "Build GLM examples" FORCE)

FetchContent_MakeAvailable(glm)
message(STATUS "GLM: 使用 FetchContent")


# nlohmann_json (总是需要)
# 禁用 natvis 文件以避免 CMake 构建错误
set(NLOHMANN_ADD_NATVIS FALSE CACHE BOOL "Disable nlohmann_json natvis" FORCE)

FetchContent_MakeAvailable(nlohmann_json)
message(STATUS "nlohmann_json: 使用 FetchContent")


# stb (总是需要)

FetchContent_MakeAvailable(stb)
# 创建 stb 接口库 (header-only)
if(NOT TARGET stb)
    add_library(stb INTERFACE IMPORTED GLOBAL)
    set_target_properties(stb PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${stb_SOURCE_DIR}"
    )
endif()
message(STATUS "stb: 使用 FetchContent")


# Tweeny (总是需要 - UI 动画)
# 暂时禁用 - CMake 版本兼容性问题
# FetchContent_MakeAvailable(tweeny)
# # 创建 tweeny 接口库 (header-only)
# # 注意：FetchContent_MakeAvailable 可能已创建目标，需要检查
# if(NOT TARGET tweeny AND NOT TARGET tweeny::tweeny)
#     add_library(tweeny INTERFACE IMPORTED GLOBAL)
#     set_target_properties(tweeny PROPERTIES
#         INTERFACE_INCLUDE_DIRECTORIES "${tweeny_SOURCE_DIR}"
#     )
#     add_library(tweeny::tweeny ALIAS tweeny)
# elseif(NOT TARGET tweeny::tweeny AND TARGET tweeny)
#     # 如果 tweeny 目标存在但没有别名，创建别名
#     add_library(tweeny::tweeny ALIAS tweeny)
# endif()
message(STATUS "Tweeny: 暂时禁用 (CMake 版本兼容性问题)")


# tinyxml2 (总是需要 - TMX 地图格式)
set(BUILD_TESTS OFF CACHE BOOL "Build tinyxml2 tests" FORCE)
FetchContent_MakeAvailable(tinyxml2)
message(STATUS "tinyxml2: 使用 FetchContent")


# Zstandard (总是需要 - TMX 压缩格式)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "Build zstd contrib" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Build zstd tests" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build static zstd library" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build shared zstd library" FORCE)
FetchContent_MakeAvailable(zstd)
message(STATUS "Zstandard: 使用 FetchContent")


# ========== 条件依赖 ==========

# SDL3 - 当启用 SDL3 音频或输入，且非 Native 模式时
# Native 模式下使用平台原生 API，不需要 SDL3
if((PRISMA_ENABLE_AUDIO_SDL3 OR PRISMA_ENABLE_INPUT_SDL3 OR PRISMA_ENABLE_RENDER_VULKAN) AND NOT PRISMA_USE_NATIVE_AUDIO AND NOT PRISMA_USE_NATIVE_INPUT)
        # SDL3 需要特殊配置来构建静态库
        set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as shared library" FORCE)
        set(SDL_STATIC ON CACHE BOOL "Build SDL3 as static library" FORCE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL3 test library" FORCE)
        # 禁用 SDL3 的测试和示例
        set(SDL_TESTS OFF CACHE BOOL "Build SDL3 tests" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL3 examples" FORCE)
        set(SDL_INSTALL_TESTS OFF CACHE BOOL "Install SDL3 tests" FORCE)
        # 禁用 SDL3 的预编译头（Android NDK 构建时可能有问题）
        set(SDL_PCH OFF CACHE BOOL "Build SDL3 with PCH" FORCE)
        FetchContent_MakeAvailable(SDL3)
        message(STATUS "SDL3: 使用 FetchContent")
endif()

# Vulkan-Headers - 当启用 Vulkan 渲染时
if(PRISMA_ENABLE_RENDER_VULKAN)
    if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
        # Android 平台使用 NDK 自带的 Vulkan，不需要额外下载
        message(STATUS "Vulkan: 使用 Android NDK 的 Vulkan")
    else()
        FetchContent_MakeAvailable(Vulkan-Headers)
        message(STATUS "Vulkan-Headers: 使用 FetchContent")

        # VMA 需要 Vulkan-Headers
        FetchContent_MakeAvailable(vma)
        message(STATUS "VMA: 使用 FetchContent")

        # vk-bootstrap 需要 Vulkan-Headers
        FetchContent_MakeAvailable(vk-bootstrap)
        message(STATUS "vk-bootstrap: 使用 FetchContent")
    endif()
endif()

# DirectX-Headers (Windows only) - 当启用 DX12 渲染或编辑器时
if(WIN32 AND (PRISMA_ENABLE_RENDER_DX12 OR PRISMA_BUILD_EDITOR))
    # 使用 FetchContent_MakeAvailable 统一处理
    # DirectX-Headers 的 CMakeLists.txt 会创建正确的目标
    FetchContent_MakeAvailable(DirectX-Headers)

    # 确保别名存在
    if(TARGET DirectX-Headers AND NOT TARGET Microsoft::DirectX-Headers)
        add_library(Microsoft::DirectX-Headers ALIAS DirectX-Headers)
    endif()

    message(STATUS "DirectX-Headers: 使用 FetchContent")

    # 调试信息：检查源码目录
    if(DEFINED DirectX-Headers_SOURCE_DIR)
        message(STATUS "  DirectX-Headers_SOURCE_DIR: ${DirectX-Headers_SOURCE_DIR}")
        message(STATUS "  Include 目录: ${DirectX-Headers_SOURCE_DIR}/include")
        message(STATUS "  DirectX 头文件路径: ${DirectX-Headers_SOURCE_DIR}/include/directx")
        # 检查目录是否存在
        if(EXISTS "${DirectX-Headers_SOURCE_DIR}/include/directx/d3dx12.h")
            message(STATUS "  ✓ d3dx12.h 存在")
        else()
            message(WARNING "  ✗ d3dx12.h 不存在!")
        endif()
    endif()

    # 调试信息：检查目标属性
    if(TARGET DirectX-Headers)
        message(STATUS "  DirectX-Headers 目标已创建")
        get_target_property(DX_INCLUDE_DIRS DirectX-Headers INCLUDE_DIRECTORIES)
        message(STATUS "  DirectX-Headers INCLUDE_DIRECTORIES: ${DX_INCLUDE_DIRS}")
    else()
        message(WARNING "  DirectX-Headers 目标未创建!")
    endif()

    if(TARGET Microsoft::DirectX-Headers)
        message(STATUS "  Microsoft::DirectX-Headers 别名已创建")
    else()
        message(WARNING "  Microsoft::DirectX-Headers 别名未创建!")
    endif()
endif()

# ImGui - Debug 模式或编辑器需要
# 检查是否是 Debug 构建或启用了编辑器
set(PRISMA_IS_DEBUG_BUILD FALSE)
# 单配置生成器检查 CMAKE_BUILD_TYPE
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PRISMA_IS_DEBUG_BUILD TRUE)
endif()
# 多配置生成器检查 CMAKE_CONFIGURATION_TYPES
if(NOT PRISMA_IS_DEBUG_BUILD)
    foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
        if(CONFIG STREQUAL "Debug")
            set(PRISMA_IS_DEBUG_BUILD TRUE)
            break()
        endif()
    endforeach()
endif()

# Debug 构建自动启用 ImGui（所有平台）
if(PRISMA_IS_DEBUG_BUILD AND NOT DEFINED PRISMA_ENABLE_IMGUI_DEBUG)
    set(PRISMA_ENABLE_IMGUI_DEBUG ON CACHE BOOL "Enable ImGui debug overlay" FORCE)
    message(STATUS "自动启用 PRISMA_ENABLE_IMGUI_DEBUG (Debug 构建)")
endif()

if(PRISMA_BUILD_EDITOR OR PRISMA_ENABLE_IMGUI_DEBUG)
    FetchContent_MakeAvailable(imgui)

    # 创建 ImGui 静态库（需要编译源文件）
    if(NOT TARGET imgui AND NOT TARGET imgui::imgui)
        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        )
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        add_library(imgui::imgui ALIAS imgui)
    elseif(NOT TARGET imgui::imgui AND TARGET imgui)
        # 如果已存在 imgui 目标但没有别名，创建别名
        add_library(imgui::imgui ALIAS imgui)
    endif()

    if(PRISMA_IS_DEBUG_BUILD AND PRISMA_ENABLE_IMGUI_DEBUG)
        message(STATUS "ImGui: 使用 FetchContent (Debug 模式调试工具)")
    else()
        message(STATUS "ImGui: 使用 FetchContent (编辑器)")
    endif()
endif()

# OpenFBX (Windows only)
if(PRISMA_BUILD_EDITOR)
    # 禁用 OpenFBX 的安装规则（避免安装阶段的错误）
    set(OPENFBX_INSTALL OFF CACHE BOOL "OpenFBX install" FORCE)
    FetchContent_MakeAvailable(openfbx)
    message(STATUS "OpenFBX: 使用 FetchContent")
endif()

message(STATUS "==============================")
message(STATUS "")

# ========== 别名创建 (统一接口) ==========

# 为 FetchContent 的库创建与 vcpkg 兼容的别名
# GLM
if(TARGET glm::glm-header-only)
    # 已有正确目标
elseif(TARGET glm)
    add_library(glm::glm-header-only ALIAS glm)
endif()

# nlohmann_json
if(TARGET nlohmann_json::nlohmann_json)
    # 已有正确目标
elseif(TARGET nlohmann_json)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()

# SDL3
if((PRISMA_ENABLE_AUDIO_SDL3 OR PRISMA_ENABLE_INPUT_SDL3 OR PRISMA_ENABLE_RENDER_VULKAN) AND NOT PRISMA_USE_NATIVE_AUDIO AND NOT PRISMA_USE_NATIVE_INPUT)
    # SDL3 在 Android 上可能创建 SDL3-static 目标
    # 为所有常见变体创建别名
    if(TARGET SDL3-static AND NOT TARGET SDL3::SDL3-static)
        add_library(SDL3::SDL3-static ALIAS SDL3-static)
    endif()
    if(TARGET SDL3_static AND NOT TARGET SDL3::SDL3-static)
        add_library(SDL3::SDL3-static ALIAS SDL3_static)
    endif()
    if(TARGET SDL3-shared AND NOT TARGET SDL3::SDL3-shared)
        add_library(SDL3::SDL3-shared ALIAS SDL3-shared)
    endif()
    if(TARGET SDL3_shared AND NOT TARGET SDL3::SDL3-shared)
        add_library(SDL3::SDL3-shared ALIAS SDL3_shared)
    endif()

    # 为了解决 install(EXPORT) 的问题，我们需要将 FetchContent
    # 获取的依赖标记为 INTERFACE，这样它们不会出现在导出集中
    # 但仍然可以被链接

    # GLM - 已是 header-only，通常不会有问题
    if(TARGET glm AND NOT TARGET glm::glm-header-only)
        set_target_properties(glm PROPERTIES
            INTERFACE_LINK_LIBRARIES ""
        )
    endif()

    # nlohmann_json
    if(TARGET nlohmann_json AND NOT TARGET nlohmann_json::nlohmann_json)
        set_target_properties(nlohmann_json PROPERTIES
            INTERFACE_LINK_LIBRARIES ""
        )
    endif()

    # ImGui - INTERFACE 库
    if(TARGET imgui AND NOT TARGET imgui::imgui)
        # 已经在上面创建了 imgui INTERFACE 目标
    endif()
endif()

# DirectX-Headers - 确保别名存在（如果 FetchContent_MakeAvailable 没有创建）
if(WIN32 AND TARGET DirectX-Headers AND NOT TARGET Microsoft::DirectX-Headers)
    add_library(Microsoft::DirectX-Headers ALIAS DirectX-Headers)
    message(STATUS "创建 Microsoft::DirectX-Headers 别名")
endif()

# ========== Android平台特殊处理 ==========

if(ANDROID)
    # Android平台优先使用系统NDK中的库
    # Vulkan是系统库，不需要额外下载
    # 如果需要，可以使用 FetchContent 获取 GLM 和 nlohmann_json
    if(NOT TARGET glm::glm-header-only)
        FetchContent_MakeAvailable(glm)
    endif()

    if(NOT TARGET nlohmann_json::nlohmann_json)
        FetchContent_MakeAvailable(nlohmann_json)
    endif()
endif()

# ========== 超分辨率依赖 / Upscaler Dependencies ==========

# FSR SDK 2.1.0 - AMD FidelityFX SDK
# 最新版本: v2.1.0 (包含 FSR Redstone) - 2025年12月
# GitHub: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
# 包含 FSR Redstone ML 神经渲染技术
if(PRISMA_ENABLE_UPSCALER_FSR)
    set(FETCHCONTENT_FIDELITYFX_SDK_DIR "${FETCHCONTENT_BASE_DIR}/FidelityFX-SDK")
    FetchContent_Declare(
        FidelityFX-SDK
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK.git
        GIT_TAG ${PRISMA_DEP_FIDELITYFX_SDK_VERSION}
        GIT_SHALLOW FALSE
    )

    # 配置 FidelityFX SDK - 必需的选项
    # 注意：这些变量必须在 FetchContent_MakeAvailable 之前设置
    set(BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set(FFX_BUILD_SDK OFF CACHE BOOL "Build SDK" FORCE)
    set(FFX_BUILD_SAMPLES OFF CACHE BOOL "Build samples" FORCE)
    set(FFX_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    set(FFX_BUILD_DOCUMENTATION OFF CACHE BOOL "Build documentation" FORCE)

    # FidelityFX 依赖选项
    set(FFX_FETCH_CONTENT OFF CACHE BOOL "Use FetchContent for dependencies" FORCE)

    # 配置外部依赖路径（使用项目已有的依赖）
    if(TARGET glm)
        set(FFX_SDK_DEPENDENCIES_GLMSHARED ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(FidelityFX-SDK)
    message(STATUS "FidelityFX SDK: 使用 FetchContent")
endif()

# DLSS 4.5 - NVIDIA Streamline SDK
# 最新版本: v2.9.0 - 2025年8月
# 支持 DLSS 4.5, Multi-Frame Generation, Ray Reconstruction
# GitHub: https://github.com/NVIDIA-RTX/Streamline
# 文档: https://developer.nvidia.com/rtx/streamline
if(PRISMA_ENABLE_UPSCALER_DLSS)
    # 检查平台支持
    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(FETCHCONTENT_STREAMLINE_DIR "${FETCHCONTENT_BASE_DIR}/Streamline")
        FetchContent_Declare(
            Streamline
            GIT_REPOSITORY https://github.com/NVIDIA-RTX/Streamline.git
            GIT_TAG ${PRISMA_DEP_STREAMLINE_VERSION}
            GIT_SHALLOW FALSE
        )

        # 配置 Streamline
        set(SL_BUILD_SAMPLES OFF CACHE BOOL "Build samples" FORCE)
        set(SL_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
        set(SL_BUILD_DOCUMENTATION OFF CACHE BOOL "Build documentation" FORCE)

        FetchContent_MakeAvailable(Streamline)
        message(STATUS "NVIDIA Streamline: 使用 FetchContent")
    else()
        message(WARNING "DLSS is only supported on Windows and Linux. Disabling...")
        set(PRISMA_ENABLE_UPSCALER_DLSS OFF CACHE BOOL "" FORCE)
    endif()
endif()
