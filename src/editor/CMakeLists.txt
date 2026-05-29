option(PRISMA_ENABLE_MCP "Enable MCP server for AI agent support" ON)

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
        core/Editor.cpp
        core/Environment.cpp
        core/CommandLineParser.cpp
        core/CommandLineEditor.cpp
        core/WebUIEditor.cpp
        core/EditorService.cpp
        core/main.cpp
        panels/EditorLayer.cpp
        panels/ProfilerPanel.cpp
        ${ImGuizmo_SOURCE_DIR}/src/ImGuizmo.cpp
        windows/ProjectSettingsWindow.cpp
        graphic/ImGuiVulkanResourceManager.cpp
        graphic/ViewportRenderPass.cpp
        # ImGui 后端
        ${IMGUI_BACKENDS}
)

# 头文件
set(EDITOR_HEADERS
        core/Editor.h
        core/Environment.h
        core/CommandLineParser.h
        core/CommandLineEditor.h
        core/WebUIEditor.h
        core/EditorService.h
        panels/EditorLayer.h
        panels/ProfilerPanel.h
        windows/ProjectSettingsWindow.h
        Export.h
        graphic/ImGuiVulkanResourceManager.h
        graphic/ViewportRenderPass.h
        UIStrings.h
)

# 1. 创建编辑器主逻辑动态库
add_library(Editor SHARED
    ${EDITOR_SOURCES}
    ${EDITOR_HEADERS}
)

# 定义导出宏
target_compile_definitions(Editor PRIVATE EDITOR_EXPORTS=1)

# 链接依赖
target_link_libraries(Editor PRIVATE
    Engine
    imgui::imgui
    SDL3-shared
)

target_include_directories(Editor PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/logger
    ${ImGuizmo_SOURCE_DIR}
    ${ImGuizmo_SOURCE_DIR}/src
)

# ========== 路径与输出配置 ==========

# 统一输出到 ${CMAKE_BINARY_DIR}/bin
set_target_properties(Editor PROPERTIES
    OUTPUT_NAME "PrismaEditor"
)

# 压制第三方库（glaze 等）的内部警告
if(MSVC)
    target_compile_options(Editor PRIVATE /wd4702)
endif()

# Windows 平台特定
if(WIN32)
    target_link_libraries(Editor PRIVATE dwmapi.lib)
endif()

# ========== MCP Server (Editor 层) ==========

if(PRISMA_ENABLE_MCP AND TARGET Editor)
    target_sources(Editor PRIVATE
        mcp/MCPServer.cpp
        mcp/MCPServer.h
        mcp/MCPTool.cpp
        mcp/MCPTool.h
        mcp/MCPSubSystem.cpp
        mcp/MCPSubSystem.h
        mcp/MCPSession.cpp
        mcp/MCPSession.h
        mcp/serialization/ComponentSerializer.cpp
        mcp/serialization/ComponentSerializer.h
        mcp/serialization/MCPJson.cpp
        mcp/serialization/MCPJson.h
        mcp/session/DeltaTracker.cpp
        mcp/session/DeltaTracker.h
        mcp/session/TokenBudget.h
        mcp/session/TokenBudget.cpp
        mcp/transport/Transport.h
        mcp/transport/TransportStdio.cpp
        mcp/transport/TransportStdio.h
        mcp/transport/TransportTCP.cpp
        mcp/transport/TransportTCP.h
        mcp/tools/SceneTools.cpp
        mcp/tools/SceneTools.h
        mcp/tools/ECSTools.cpp
        mcp/tools/ECSTools.h
        mcp/tools/EngineTools.cpp
        mcp/tools/EngineTools.h
        mcp/tools/DebugTools.cpp
        mcp/tools/DebugTools.h
        mcp/tools/AssetTools.cpp
        mcp/tools/AssetTools.h
        mcp/tools/EditorTools.cpp
        mcp/tools/EditorTools.h
        mcp/tools/GameTools.cpp
        mcp/tools/GameTools.h
    )

    target_include_directories(Editor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # xxhash for hash-based delta tracking
    add_library(xxhash STATIC "${PROJECT_SOURCE_DIR}/.dependencies/xxhash-src/xxhash.c")
    target_include_directories(xxhash PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/xxhash-src")
    set_target_properties(xxhash PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(Editor PRIVATE xxhash)

    target_compile_definitions(Editor PRIVATE PRISMA_ENABLE_MCP=1)

    # TransportTCP 需要 Winsock2 (Windows)
    if(WIN32)
        target_link_libraries(Editor PRIVATE ws2_32)
    endif()
endif()
