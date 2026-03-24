# ImGui 后端源文件 (根据平台条件添加)
# -----------------------------------------------------------------------
# [修复] 不再向 Editor 目标添加 imgui_impl_vulkan.cpp / imgui_impl_sdl3.cpp
#
# 目的：避免 ImGui Vulkan 后端被编译两次。
#
# 问题根源：
#   FetchThirdPartyDeps.cmake 已将 imgui_impl_vulkan.cpp 编译进静态库 `imgui`，
#   Engine.dll 通过 PUBLIC 链接将其传递给 Editor.dll。
#   若 Editor.dll 再单独编译一份 imgui_impl_vulkan.cpp，则 ImGui 后端的
#   静态全局变量（包括 g_FunctionsLoaded 和 BackendRendererUserData）在两
#   个编译单元中各自独立存在。ImGui_ImplVulkan_Init 向一个副本注册后端数据，
#   而 ImGui_ImplVulkan_RenderDrawData 从另一个副本的 GetBackendData() 读取，
#   得到全零数据，字体纹理 TexID 始终为 ImTextureID_Invalid（0xFFFFFFFFFFFFFFFF），
#   传入 vkCmdBindDescriptorSets 引发访问冲突崩溃。
#
# 修复过程：
#   Editor 只链接 imgui 静态库（已包含所有 backends），不再独立编译任何 backend。
# -----------------------------------------------------------------------
set(IMGUI_BACKENDS)

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
        EditorLayer.h
        ProjectSettingsWindow.h
        CommandLineParser.h
        CommandLineEditor.h
        Environment.h
        Export.h
)

# 1. 创建编辑器主逻辑动态库
add_library(Editor SHARED
    ${EDITOR_SOURCES}
    ${EDITOR_HEADERS}
)

# 定义导出宏
target_compile_definitions(Editor PRIVATE EDITOR_EXPORTS=1)

# 链接依赖
target_link_libraries(Editor PRIVATE Engine imgui::imgui)

# 2. 创建编辑器启动器 (EXE)
add_executable(PrismaEditor main.cpp)

# 链接到编辑器 DLL 和 引擎 DLL
target_link_libraries(PrismaEditor PRIVATE Editor Engine)

# ========== 路径与输出配置 ==========

# 统一输出到 ${CMAKE_BINARY_DIR}/bin
set_target_properties(Editor PROPERTIES
    OUTPUT_NAME "PrismaEditor"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

set_target_properties(PrismaEditor PROPERTIES
    OUTPUT_NAME "Prisma"
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
