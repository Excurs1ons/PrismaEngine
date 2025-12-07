# 引擎核心库

# 收集源文件
file(GLOB_RECURSE ENGINE_SOURCES 
    "*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "*.h"
)

# 创建静态库
add_library(Engine STATIC 
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    packing/pipeline.h
)

# 设置包含目录
target_include_directories(Engine
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/resource
        ${CMAKE_CURRENT_SOURCE_DIR}/audio
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

# 设置编译定义
target_compile_definitions(Engine 
    PRIVATE ENGINE_EXPORTS
    PUBLIC _UNICODE UNICODE
)

# 设置编译选项
target_compile_options(Engine PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 链接vcpkg管理的系统库
find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
#find_package(openfbx CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)

target_link_libraries(Engine PUBLIC 
    SDL3::SDL3
    Vulkan::Vulkan
    nlohmann_json::nlohmann_json
    imgui::imgui
#    openfbx::openfbx
    GTest::gtest
    GTest::gtest_main
    Microsoft::DirectX-Headers
)

# 链接系统库
if(WIN32)
    target_link_libraries(Engine PRIVATE xaudio2
        d3d12
        dxgi
        d3dcompiler
    )
else (ANDROID)
    target_link_libraries(Engine PRIVATE
        log
    )

endif()

# 设置公共头文件
set_target_properties(Engine PROPERTIES
    PUBLIC_HEADER "${ENGINE_HEADERS}"
)

# 设置输出名称
set_target_properties(Engine PROPERTIES
    OUTPUT_NAME "PrismaEngine"
)