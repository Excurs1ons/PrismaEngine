# 游戏模块

# 源文件
set(GAME_SOURCES
    Game.cpp
    pch.cpp
)

set(GAME_HEADERS
    framework.h
    pch.h
)

# 创建动态库
add_library(Game SHARED
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

# 包含目录
target_include_directories(Game
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)

# 链接依赖
target_link_libraries(Game 
    PRIVATE Engine
)

# 编译选项
target_compile_options(Game PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 预编译头设置
if(MSVC)
    target_precompile_headers(Game PRIVATE pch.h)
endif()

# 编译定义
target_compile_definitions(Game
    PRIVATE GAME_EXPORTS _UNICODE UNICODE
)

# 设置输出名称
set_target_properties(Game PROPERTIES
    OUTPUT_NAME "PrismaGame"
)