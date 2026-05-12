# 运行时可执行程序
# 启动器通过运行时加载 (LoadLibrary / dlopen) 方式加载 Engine DLL，
# 不再编译期链接 Engine 导入库。

set(LAUNCHER_SOURCES core/LauncherMain.cpp)

# 创建可执行程序
add_executable(Launcher
    ${LAUNCHER_SOURCES}
)

set_target_properties(Launcher PROPERTIES
    OUTPUT_NAME "PrismaLauncher"
)

# ====================================================================
# PRISMA_RUNTIME_LOAD_ENGINE — 运行时加载 Engine DLL
#
# PrismaLauncher 不再链接 Engine 导入库。Engine.dll 在 main() 中
# 通过 LoadLibrary / dlopen 运行时解析，C API 函数通过
# GetProcAddress / dlsym 获取。
#
# 优点：
#   - PrismaLauncher 完全独立于 Engine DLL，可独立更新
#   - 启动时动态选择 Engine DLL 版本 (debug/release)
#   - 更清晰的模块边界
# ====================================================================
# 添加 IEngine.h 纯虚接口的包含路径（仅此一个头文件，无 Engine 链接依赖）
target_include_directories(Launcher PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/interfaces
)

message(STATUS "Launcher: 运行时加载 Engine DLL + Game/Editor 插件")

# 编译选项
target_compile_options(Launcher PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 编译定义
target_compile_definitions(Launcher
    PRIVATE _UNICODE UNICODE
)
