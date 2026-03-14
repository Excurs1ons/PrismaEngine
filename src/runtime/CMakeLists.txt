# 运行时可执行程序
# Runtime 在 Web 平台使用静态链接模式，不依赖动态加载

if(ANDROID)
    set(RUNTIME_SOURCES android/AndroidRuntime.cpp)
elseif(EMSCRIPTEN)
    set(RUNTIME_SOURCES web/WebRuntime.cpp)
elseif(WIN32)
    set(RUNTIME_SOURCES windows/WindowsRuntime.cpp)
    set(RUNTIME_RESOURCES
            resources/windows/icons/Runtime.ico
            resources/windows/icons/small.ico)
elseif(APPLE)
    message(FATAL_ERROR "尚不支持此平台:APPLE")
elseif(LINUX)
    set(RUNTIME_SOURCES linux/LinuxRuntime.cpp)
endif()

# 创建可执行程序
add_executable(PrismaLauncher
    ${RUNTIME_SOURCES}
)

# 只链接 Engine
target_link_libraries(PrismaLauncher
    PRIVATE Engine
)

if(EMSCRIPTEN)
    if(TARGET Game)
        target_link_libraries(PrismaLauncher PRIVATE Game)
    else()
        message(FATAL_ERROR "WebAssembly 构建需要 Game 目标，请关闭 PRISMA_BUILD_EDITOR 并启用 Game 模块")
    endif()

    target_link_options(PrismaLauncher PRIVATE
        "-sALLOW_MEMORY_GROWTH=1"
        "-sUSE_SDL=3"
        "-sWASM=1"
        "-sASSERTIONS=1"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "--shell-file=${CMAKE_SOURCE_DIR}/src/runtime/web/shell_minimal.html"
    )

    if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
        target_link_options(PrismaLauncher PRIVATE "--preload-file" "${CMAKE_SOURCE_DIR}/assets@/Assets")
    endif()

    set_target_properties(PrismaLauncher PROPERTIES
        OUTPUT_NAME "PrismaLauncher"
        SUFFIX ".html"
    )
endif()

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

# 安装规则
#install(TARGETS Runtime
#    RUNTIME DESTINATION bin
#)
