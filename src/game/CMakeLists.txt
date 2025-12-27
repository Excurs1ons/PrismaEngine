# 游戏模块

# 源文件
set(GAME_SOURCES
    Game.cpp
    pch.cpp
)

set(GAME_HEADERS
    framework.h
    pch.h
    Game.h
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
)

# 链接依赖
target_link_libraries(Game
    PRIVATE Engine
)

# 链接 DirectX-Headers (由 FetchContent.cmake 管理)
if(TARGET Microsoft::DirectX-Headers)
    target_link_libraries(Game PRIVATE Microsoft::DirectX-Headers)
elseif(TARGET DirectX-Headers)
    target_link_libraries(Game PRIVATE DirectX-Headers)
endif()

# Windows 系统库
target_link_libraries(Game
    PRIVATE
    d3d12
    dxgi
    d3dcompiler
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

# 安装规则
install(TARGETS Game
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)