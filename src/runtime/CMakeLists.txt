# 运行时可执行程序
# 统一使用跨平台实现，基于 SDL3 动态加载

set(RUNTIME_SOURCES Runtime.cpp)

# 创建可执行程序
add_executable(PrismaLauncher
    ${RUNTIME_SOURCES}
)

# 只链接 Engine
target_link_libraries(PrismaLauncher
    PRIVATE Engine
)

message(STATUS "Launcher: 独立构建 - 运行时动态加载 Game/Editor")

# 编译选项
target_compile_options(PrismaLauncher PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 编译定义
target_compile_definitions(PrismaLauncher
    PRIVATE _UNICODE UNICODE
)

# 设置输出名称
set_target_properties(PrismaLauncher PROPERTIES
    OUTPUT_NAME "PrismaLauncher"
)
