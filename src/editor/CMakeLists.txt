# 编辑器模块

# 源文件
set(EDITOR_SOURCES
        Editor.cpp
        ProjectSettingsWindow.cpp
)

set(EDITOR_HEADERS
        Editor.h
        ProjectSettingsWindow.h
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
)

# 链接依赖
# DirectX-Headers 由 FetchContent.cmake 管理
target_link_libraries(Editor
    PRIVATE Engine
)

# 修复 MSVC 下找不到 Engine.lib 的问题
if(MSVC)
    # 尝试多种可能的路径
    target_link_directories(Editor PRIVATE
        ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
        ${CMAKE_BINARY_DIR}/src/engine/$<CONFIG>
    )
endif()

# 链接 SDL3 (如果可用)
if(TARGET SDL3::SDL3-static)
    target_link_libraries(Editor PRIVATE SDL3::SDL3-static)
elseif(TARGET SDL3::SDL3)
    target_link_libraries(Editor PRIVATE SDL3::SDL3)
elseif(TARGET SDL3)
    target_link_libraries(Editor PRIVATE SDL3)
endif()

# 链接 DirectX-Headers
if(TARGET Microsoft::DirectX-Headers)
    target_link_libraries(Editor PRIVATE Microsoft::DirectX-Headers)
elseif(TARGET DirectX-Headers)
    target_link_libraries(Editor PRIVATE DirectX-Headers)
else()
    find_package(directx-headers CONFIG REQUIRED)
    target_link_libraries(Editor PRIVATE Microsoft::DirectX-Headers)
endif()

# Windows 系统库
target_link_libraries(Editor
    PRIVATE
    d3d12
    dxgi
    d3dcompiler
)

# 编译选项
target_compile_options(Editor PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 编译定义
target_compile_definitions(Editor
    PRIVATE _UNICODE UNICODE
    PRIVATE EDITOR_EXPORTS=1
)

# 设置输出名称
set_target_properties(Editor PROPERTIES
    OUTPUT_NAME "PrismaEditor"
)

# 安装规则
install(TARGETS Editor
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)