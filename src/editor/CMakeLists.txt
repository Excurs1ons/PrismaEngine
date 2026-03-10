# 编辑器模块

# ImGui 后端源文件 (根据平台条件添加)
set(IMGUI_BACKENDS)
if(WIN32)
    list(APPEND IMGUI_BACKENDS ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp)
endif()
if(PRISMA_ENABLE_RENDER_VULKAN)
    list(APPEND IMGUI_BACKENDS ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
endif()
# SDL3 后端 - 仅在使用 SDL3 时添加，但排除 Android 平台（Android 不使用 SDL3）
if(NOT ANDROID AND (PRISMA_ENABLE_AUDIO_SDL3 OR PRISMA_USE_NATIVE_INPUT STREQUAL "OFF"))
    if(EXISTS ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
        list(APPEND IMGUI_BACKENDS ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
    endif()
endif()

# 源文件
set(EDITOR_SOURCES
        Editor.cpp
        ProjectSettingsWindow.cpp
        CommandLineParser.cpp
        CommandLineEditor.cpp
        Environment.cpp
        # ImGui 后端
        ${IMGUI_BACKENDS}
)

# 头文件
set(EDITOR_HEADERS
        Editor.h
        ProjectSettingsWindow.h
        CommandLineParser.h
        CommandLineEditor.h
        Environment.h
        Export.h
)

# 1. 创建编辑器主逻辑动态库 (重命名为 PrismaEditorCore 以避免冲突)
add_library(Editor SHARED
    ${EDITOR_SOURCES}
    ${EDITOR_HEADERS}
)

# 定义导出宏
target_compile_definitions(Editor PRIVATE EDITOR_EXPORTS=1)

# 链接依赖
target_link_libraries(Editor PRIVATE Engine)

# 2. 创建编辑器启动器 (EXE)
add_executable(PrismaEditor main.cpp)

# 链接到编辑器 DLL 和 引擎 DLL
target_link_libraries(PrismaEditor PRIVATE Editor Engine)

# ========== 路径与输出配置 ==========

# 统一输出到 ${CMAKE_BINARY_DIR}/bin
set_target_properties(Editor PROPERTIES
    OUTPUT_NAME "PrismaEditorCore"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

set_target_properties(PrismaEditor PROPERTIES
    OUTPUT_NAME "PrismaEditor"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 自动拷贝 SDL3.dll 到输出目录 (Windows)
if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET PrismaEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3-shared>
        $<TARGET_FILE_DIR:PrismaEditor>
        COMMENT "Copying SDL3.dll to bin directory"
    )
endif()

# 设置 VS 调试属性：工作目录设为项目根目录以加载资源
if(MSVC)
    set_target_properties(PrismaEditor PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()

# ========== 跨平台依赖 ==========

if(TARGET SDL3::SDL3-shared)
    target_link_libraries(Editor PRIVATE SDL3::SDL3-shared)
elseif(TARGET SDL3)
    target_link_libraries(Editor PRIVATE SDL3)
endif()

# Windows 平台特定
if(WIN32)
    target_link_libraries(Editor PRIVATE dwmapi.lib)
    # 添加入口点类型 (图形界面)
    # set_target_properties(PrismaEditor PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
