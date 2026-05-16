# FetchContent.cmake
# 使用 CMake FetchContent 替代 vcpkg 管理依赖

include(FetchContent)
include(${CMAKE_CURRENT_LIST_DIR}/DependencyVersions.cmake OPTIONAL)

# ========== 全局流量保护与依赖共享逻辑 ==========

set(PRISMA_GLOBAL_DEPS_DIR "${PROJECT_SOURCE_DIR}/.dependencies")
set(FETCHCONTENT_BASE_DIR "${PRISMA_GLOBAL_DEPS_DIR}" CACHE PATH "全局依赖缓存目录" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

# 静音 FetchContent 自身的输出
set(FETCHCONTENT_QUIET ON CACHE BOOL "" FORCE)

# 辅助宏：声明依赖项，实现真正的离线优先
macro(Prisma_Declare_Dependency NAME REPO TAG)
    set(DEP_SOURCE_DIR "${PRISMA_GLOBAL_DEPS_DIR}/${NAME}-src")
    set(DEP_SOURCE_DIR_ALT "${PRISMA_GLOBAL_DEPS_DIR}/${NAME}")
    
    set(ACTUAL_SOURCE_DIR "")
    set(REPO_MARKERS "CMakeLists.txt" "LICENSE" "LICENSE.txt" "README.md" "README" "include")

    foreach(marker ${REPO_MARKERS})
        if(EXISTS "${DEP_SOURCE_DIR}/${marker}")
            set(ACTUAL_SOURCE_DIR "${DEP_SOURCE_DIR}")
            break()
        elseif(EXISTS "${DEP_SOURCE_DIR_ALT}/${marker}")
            set(ACTUAL_SOURCE_DIR "${DEP_SOURCE_DIR_ALT}")
            break()
        endif()
    endforeach()

    if(ACTUAL_SOURCE_DIR)
        message(STATUS "  [SHARED]  ${NAME} -> ${ACTUAL_SOURCE_DIR}")
        FetchContent_Declare(${NAME} SOURCE_DIR "${ACTUAL_SOURCE_DIR}" BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/${NAME}-build")
    else()
        message(STATUS "  [FETCH]   ${NAME} <- ${REPO}")
        FetchContent_Declare(${NAME} GIT_REPOSITORY ${REPO} GIT_TAG ${TAG} GIT_SHALLOW ON SOURCE_DIR "${DEP_SOURCE_DIR}" BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/${NAME}-build")
    endif()
endmacro()

# ========== 依赖项声明 ==========

Prisma_Declare_Dependency(glm https://github.com/g-truc/glm.git ${PRISMA_DEP_GLM_VERSION})
# nlohmann_json 依赖已移除 (正在迁移至 Glaze)
Prisma_Declare_Dependency(stb https://github.com/nothings/stb.git ${PRISMA_DEP_STB_VERSION})
set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
Prisma_Declare_Dependency(tinyxml2 https://github.com/leethomason/tinyxml2.git ${PRISMA_DEP_TINYXML2_VERSION})
Prisma_Declare_Dependency(zstd https://github.com/facebook/zstd.git ${PRISMA_DEP_ZSTD_VERSION})
Prisma_Declare_Dependency(SDL3 https://github.com/libsdl-org/SDL.git ${PRISMA_DEP_SDL3_VERSION})
Prisma_Declare_Dependency(Vulkan-Headers https://github.com/KhronosGroup/Vulkan-Headers.git ${PRISMA_DEP_VULKAN_HEADERS_VERSION})
Prisma_Declare_Dependency(vma https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git ${PRISMA_DEP_VMA_VERSION})
Prisma_Declare_Dependency(vk-bootstrap https://github.com/charles-lunarg/vk-bootstrap.git ${PRISMA_DEP_VK_BOOTSTRAP_VERSION})

Prisma_Declare_Dependency(imgui https://github.com/ocornut/imgui.git ${PRISMA_DEP_IMGUI_VERSION})

# xxhash - 极快哈希 (MCP 增量追踪)
# 编译为静态库，禁用测试
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
set(DISPATCH_EXAMPLES OFF CACHE BOOL "" FORCE)
Prisma_Declare_Dependency(xxhash https://github.com/Cyan4973/xxHash.git ${PRISMA_DEP_XXHASH_VERSION})
FetchContent_MakeAvailable(xxhash)

# Glaze - 极速 JSON 库 (用于替代 nlohmann/json 作为最佳实践)
Prisma_Declare_Dependency(glaze https://github.com/stephenberry/glaze.git ${PRISMA_DEP_GLAZE_VERSION})
FetchContent_MakeAvailable(glaze)

# ========== 依赖项加载与配置 ==========

# 禁用所有不需要的测试、示例和程序
set(GLM_QUIET ON CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NLOHMANN_ADD_NATVIS OFF CACHE BOOL "" FORCE)
set(SDL_ALSA OFF CACHE BOOL "" FORCE)
set(NLOHMANN_ADD_NATVIS OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
# [重要] SDL3 链接配置
# 在 Windows 的插件化架构（多 DLL）中，必须使用 SHARED (DLL) 链接。
# 原因：SDL3 内部维护了大量的全局状态（如窗口句柄、事件队列、视频驱动上下文）。
#       如果使用静态链接，Engine.dll 和 Editor.dll 各自会持有一份 SDL3 副本，
#       导致状态分裂，跨 DLL 传递 SDL 指针时会因句柄找不到而闪退。
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)

# 强制开启相关依赖的位置无关代码 (PIC)
set(VK_BOOTSTRAP_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)

# 针对 tinyxml2 的专项屏蔽
set(TINYXML2_BUILD_TESTING OFF CACHE BOOL "" FORCE)

# 暂时提升消息等级以压制第三方库的繁杂输出
set(OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
set(CMAKE_MESSAGE_LOG_LEVEL WARNING)
set(CMAKE_WARN_DEPRECATED OFF)

# 加载依赖 (使用 EXCLUDE_FROM_ALL 进一步隔离不需要的 target)
FetchContent_MakeAvailable(glm stb tinyxml2 zstd)

# STB 总是作为接口库处理
if(NOT TARGET stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE "${stb_SOURCE_DIR}")
endif()
if(NOT TARGET stb::stb)
    add_library(stb::stb ALIAS stb)
endif()

if(PRISMA_BUILD_EDITOR OR PRISMA_ENABLE_RENDER_VULKAN)
    FetchContent_MakeAvailable(SDL3)
    foreach(sdl_target SDL3-shared SDL3-static SDL3_test SDL_uclibc)
        if(TARGET ${sdl_target})
            if(MSVC)
                target_compile_options(${sdl_target} PRIVATE /W0)
            else()
                target_compile_options(${sdl_target} PRIVATE -w)
            endif()
        endif()
    endforeach()
endif()

if(PRISMA_ENABLE_RENDER_VULKAN)
    FetchContent_MakeAvailable(Vulkan-Headers vma vk-bootstrap)
    Prisma_Declare_Dependency(spirv-reflect https://github.com/KhronosGroup/SPIRV-Reflect.git ${PRISMA_DEP_SPIRV_REFLECT_VERSION})
    set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
    set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(SPIRV_REFLECT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(spirv-reflect)
endif()

# ImGui 静态库创建
FetchContent_MakeAvailable(imgui)

set(IMGUI_CORE_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
)

# Windows 后端
if(WIN32)
    list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp)
endif()

# Vulkan 后端 - 跨平台
if(PRISMA_ENABLE_RENDER_VULKAN)
    list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
endif()

# SDL3 后端 - 跨平台（包括 Windows）
if(EXISTS ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
    list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
endif()

if(NOT TARGET imgui)
    add_library(imgui STATIC ${IMGUI_CORE_SOURCES})
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    
    # 强力注入包含路径
    target_include_directories(imgui SYSTEM PUBLIC 
        "${PRISMA_GLOBAL_DEPS_DIR}/SDL3-src/include"
        "${PRISMA_GLOBAL_DEPS_DIR}/Vulkan-Headers-src/include"
    )

    add_library(imgui::imgui ALIAS imgui)
endif()

# 确保 GLM ALIAS targets 存在
if(TARGET glm AND NOT TARGET glm::glm)
    add_library(glm::glm ALIAS glm)
endif()
if(TARGET glm AND NOT TARGET glm::glm-header-only)
    add_library(glm::glm-header-only ALIAS glm)
endif()

# 恢复消息等级
set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})
set(CMAKE_WARN_DEPRECATED ON)

# 确保 VMA ALIAS targets 存在（类似 GLM）
if(TARGET vma AND NOT TARGET vma::VulkanMemoryAllocator)
    add_library(vma::VulkanMemoryAllocator ALIAS vma)
endif()
if(TARGET vma AND NOT TARGET vma::VMA)
    add_library(vma::VMA ALIAS vma)
endif()
