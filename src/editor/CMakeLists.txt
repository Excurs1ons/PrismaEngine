# 编辑器模块

# 源文件
set(EDITOR_SOURCES
        Editor.cpp
        ProjectSettingsWindow.cpp
        CommandLineParser.cpp
        CommandLineEditor.cpp
        Environment.cpp
        # 直接包含 ImGui 后端实现，确保 DLL 链接成功
        ${CMAKE_SOURCE_DIR}/.dependencies/imgui-src/backends/imgui_impl_sdl3.cpp
        ${CMAKE_SOURCE_DIR}/.dependencies/imgui-src/backends/imgui_impl_vulkan.cpp
        ${CMAKE_SOURCE_DIR}/.dependencies/imgui-src/backends/imgui_impl_win32.cpp
)

set(EDITOR_HEADERS
        Editor.h
        ProjectSettingsWindow.h
        CommandLineParser.h
        CommandLineEditor.h
        Environment.h
)

# 创建动态库
add_library(Editor SHARED
    ${EDITOR_SOURCES}
    ${EDITOR_HEADERS}
)

# 包含目录
target_include_directories(Editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine/resource
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine/audio
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine/graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine/platform
        # ImGui 后端包含目录
        ${CMAKE_SOURCE_DIR}/.dependencies/imgui-src
        ${CMAKE_SOURCE_DIR}/.dependencies/imgui-src/backends
)

# 链接依赖
target_link_libraries(Editor
    PRIVATE Engine
)

# Engine 作为静态库链接时，需要定义 ENGINE_BUILD_STATIC
# ========== 编译定义 ==========
target_compile_definitions(Editor
    PRIVATE EDITOR_EXPORTS=1
    PRIVATE PRISMA_USE_SDL3=1
    PRIVATE PRISMA_ENABLE_RENDER_VULKAN=1
)

# ========== 平台特定依赖配置 ==========

# Windows 平台
if(WIN32)
    # 修复 MSVC 下找不到 Engine.lib 的问题
    if(MSVC)
        target_link_directories(Editor PRIVATE
            ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
            ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
            ${CMAKE_BINARY_DIR}/src/engine/$<CONFIG>
        )
    endif()

    # Windows 系统库
    find_package(Vulkan REQUIRED)
    target_link_libraries(Editor PRIVATE
        Vulkan::Vulkan
    )

    # Windows 编译选项
    target_compile_options(Editor PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
    )

    target_compile_definitions(Editor PRIVATE _UNICODE UNICODE)
endif()
# Linux 平台
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Vulkan 依赖
    if(PRISMA_ENABLE_RENDER_VULKAN)
        # Vulkan-Headers
        if(TARGET Vulkan::Headers)
            target_link_libraries(Editor PRIVATE Vulkan::Headers)
        elseif(TARGET Vulkan-Headers)
            target_link_libraries(Editor PRIVATE Vulkan-Headers)
        else()
            find_package(Vulkan QUIET)
            if(Vulkan_FOUND)
                target_include_directories(Editor PRIVATE ${Vulkan_INCLUDE_DIRS})
                target_link_libraries(Editor PRIVATE ${Vulkan_LIBRARIES})
            endif()
        endif()

        # VMA (Vulkan Memory Allocator)
        if(TARGET GPUOpen::VulkanMemoryAllocator)
            target_link_libraries(Editor PRIVATE GPUOpen::VulkanMemoryAllocator)
        elseif(TARGET vma)
            target_link_libraries(Editor PRIVATE vma)
        endif()

        # vk-bootstrap
        if(TARGET vk-bootstrap::vk-bootstrap)
            target_link_libraries(Editor PRIVATE vk-bootstrap::vk-bootstrap)
        elseif(TARGET vk-bootstrap)
            target_link_libraries(Editor PRIVATE vk-bootstrap)
        endif()
    endif()

    # Linux 系统库
    target_link_libraries(Editor PRIVATE
        dl
        pthread
    )

    # Linux 编译选项
    target_compile_options(Editor PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# ========== 跨平台依赖 ==========

# 链接 SDL3 (跨平台)
if(TARGET SDL3::SDL3-static)
    target_link_libraries(Editor PRIVATE SDL3::SDL3-static)
elseif(TARGET SDL3_static)
    target_link_libraries(Editor PRIVATE SDL3_static)
elseif(TARGET SDL3::SDL3-shared)
    target_link_libraries(Editor PRIVATE SDL3::SDL3-shared)
elseif(TARGET SDL3_shared)
    target_link_libraries(Editor PRIVATE SDL3_shared)
elseif(TARGET SDL3::SDL3)
    target_link_libraries(Editor PRIVATE SDL3::SDL3)
elseif(TARGET SDL3)
    target_link_libraries(Editor PRIVATE SDL3)
endif()

# 链接 ImGui (跨平台)
if(TARGET imgui::imgui)
    target_link_libraries(Editor PRIVATE imgui::imgui)
elseif(TARGET imgui)
    target_link_libraries(Editor PRIVATE imgui)
endif()

# ========== 编译定义 ==========
target_compile_definitions(Editor
    PRIVATE EDITOR_EXPORTS=1
)

# 设置输出名称
set_target_properties(Editor PROPERTIES
    OUTPUT_NAME "PrismaEditor"
)

# 安装规则 (仅在启用 install 时生效)
if(PRISMA_ENABLE_INSTALL)
    install(TARGETS Editor
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# ========== 编辑器可执行文件 ==========

# 编辑器主程序（支持 GUI 和 CLI 模式）
add_executable(PrismaEditor
    main.cpp
)

target_link_libraries(PrismaEditor
    PRIVATE Editor
    PRIVATE Engine
)

# 包含目录
target_include_directories(PrismaEditor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine
)

# 设置输出
set_target_properties(PrismaEditor PROPERTIES
    OUTPUT_NAME "PrismaEditor"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则 (仅在启用 install 时生效)
if(PRISMA_ENABLE_INSTALL)
    install(TARGETS PrismaEditor
        RUNTIME DESTINATION bin
    )
endif()