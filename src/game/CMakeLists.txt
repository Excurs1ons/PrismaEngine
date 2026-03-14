# PrismaCraft 示例游戏项目

# 源文件
set(GAME_SOURCES
    PrismaCraft.cpp
)

set(GAME_HEADERS
    # 目前不需要额外头文件
)

# 构建为动态库 (Plugin)
add_library(Game SHARED
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

message(STATUS "Game: 构建为动态库 (SHARED)")

# 包含目录
target_include_directories(Game
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/engine
)

# 链接依赖 (必须链接到 Engine)
target_link_libraries(Game
    PRIVATE Engine
)

# 编译选项
target_compile_options(Game PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 编译定义
target_compile_definitions(Game
    PRIVATE _UNICODE UNICODE
)

# 设置输出名称为 PrismaCraft
set_target_properties(Game PROPERTIES
    OUTPUT_NAME "PrismaCraft"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# 安装规则
install(TARGETS Game
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
