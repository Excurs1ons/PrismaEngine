# OutputDirectories.cmake
# 输出目录配置

# 构建目录结构: build/{system}-{platform}-{build_type}/bin|lib

# ========== 默认安装前缀 ==========

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

# ========== 输出目录配置 ==========

# 对于多配置生成器（如 Visual Studio），为每种配置设置不同的输出目录
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} UPPER_CONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UPPER_CONFIG} ${CMAKE_BINARY_DIR}/bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${UPPER_CONFIG} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${UPPER_CONFIG} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
endforeach()

# 单配置生成器的默认设置（Ninja、Makefile 等）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ========== 输出目录信息输出 ==========

message(STATUS "")
message(STATUS "=== Output Directories ===")

if(CMAKE_CONFIGURATION_TYPES)
    # 多配置生成器
    message(STATUS "Multi-config generator detected:")
    foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG} UPPER_CONFIG)
        message(STATUS "  ${CONFIG}:")
        message(STATUS "    Runtime: bin/${CONFIG}")
        message(STATUS "    Library: lib/${CONFIG}")
    endforeach()
else()
    # 单配置生成器
    message(STATUS "Single-config generator:")
    message(STATUS "  Runtime: bin")
    message(STATUS "  Library: lib")
endif()

message(STATUS "  Install: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==========================")
message(STATUS "")
