# Blender集成模块
# 提供原生.blender格式支持和实时跨进程编辑

# 源文件
set(BLENDER_INTEGRATION_SOURCES
    ipc/BlenderIPCServer.cpp
    importers/BlenderSceneImporter.cpp
    tasks/ParallelTaskSystem.cpp
    editor/BlenderPanel.cpp
    renderer/BlenderMesh.cpp
    renderer/BlenderMaterial.cpp
)

# 头文件
set(BLENDER_INTEGRATION_HEADERS
    ipc/BlenderIPCServer.h
    ipc/SceneData.h
    ipc/BlenderProtocol.h
    ipc/BlenderRPCServer.h
    importers/BlenderSceneImporter.h
    tasks/ParallelTaskSystem.h
    editor/BlenderPanel.h
    renderer/BlenderMesh.h
    renderer/BlenderMaterial.h
)

# 创建静态库
add_library(BlenderIntegration STATIC
    ${BLENDER_INTEGRATION_SOURCES}
)

# 包含目录
target_include_directories(BlenderIntegration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ipc
    ${CMAKE_CURRENT_SOURCE_DIR}/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks
    ${CMAKE_CURRENT_SOURCE_DIR}/editor
)

# 链接依赖
target_link_libraries(BlenderIntegration PRIVATE
    Engine
    nlohmann_json::nlohmann_json
)

# 定义导出宏
target_compile_definitions(BlenderIntegration PRIVATE
    BLENDER_INTEGRATION_EXPORTS=1
    PRISMA_ENABLE_BLENDER_INTEGRATION=1
)

# 编译器特性
if(MSVC)
    target_compile_options(BlenderIntegration PRIVATE
        /W4
        /wd4251  # 禁用dll接口警告
        /wd4275  # 禁用非dll接口警告
    )
else()
    target_compile_options(BlenderIntegration PRIVATE
        -Wall
        -Wextra
        -Werror
    )
endif()

# 平台特定设置
if(WIN32)
    target_link_libraries(BlenderIntegration PRIVATE
        ws2_32.lib
    )
endif()

# 输出设置
set_target_properties(BlenderIntegration PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    POSITION_INDEPENDENT_CODE ON
)

# 安装规则
install(TARGETS BlenderIntegration
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blender_plugin
    DESTINATION share/prisma/blender
    FILES_MATCHING PATTERN "*.py"
)