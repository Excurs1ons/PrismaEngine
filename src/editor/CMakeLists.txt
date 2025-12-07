# 编辑器模块

# 源文件
set(EDITOR_SOURCES
        ApplicationEditor.cpp
        Editor.cpp
        WindowsUtils.cpp
)

set(EDITOR_HEADERS
        ApplicationEditor.h
        WindowsUtils.h
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
        ${CMAKE_CURRENT_SOURCE_DIR}/../core
        ${CMAKE_CURRENT_SOURCE_DIR}/../core/resource
        ${CMAKE_CURRENT_SOURCE_DIR}/../core/audio
        ${CMAKE_CURRENT_SOURCE_DIR}/../core/graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/../core/platform
)

# 查找并链接 DirectX 12 (使用 Windows SDK)
find_package(directx-headers CONFIG REQUIRED)

# 链接依赖
target_link_libraries(Editor 
    PRIVATE Engine
    Microsoft::DirectX-Headers
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
)

# 设置输出名称
set_target_properties(Editor PROPERTIES
    OUTPUT_NAME "PrismaEditor"
)