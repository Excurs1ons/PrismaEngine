# 运行时可执行程序
if (ANDROID)
    # 源文件
    set(RUNTIME_SOURCES android/AndroidRuntime.cpp)
elseif (WIN32)
    # 源文件
    set(RUNTIME_SOURCES WindowsRuntime.cpp)
    set(RUNTIME_RESOURCES
            ../../resources/runtime/windows/Runtime.ico
            ../../resources/runtime/windows/small.ico)
elseif (APPLE)
    message(FATAL_ERROR "尚不支持此平台:APPLE")
elseif (LINUX)
    set(RUNTIME_SOURCES LinuxRuntime.cpp)
endif ()

# 创建可执行程序
add_executable(Runtime
    ${RUNTIME_SOURCES}
    ${RUNTIME_RESOURCES}
)

# 包含目录
target_include_directories(Runtime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine
)
# 需要在子目录
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../engine)
# 链接依赖
target_link_libraries(Runtime 
    PRIVATE Engine
)

# 编译选项
target_compile_options(Runtime PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 编译定义
target_compile_definitions(Runtime
    PRIVATE _UNICODE UNICODE
)

# 设置输出名称
set_target_properties(Runtime PROPERTIES
    OUTPUT_NAME "PrismaRuntime"
)

# 安装规则
#install(TARGETS Runtime
#    RUNTIME DESTINATION bin
#)