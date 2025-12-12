# FindMono.cmake - 查找Mono运行时

find_path(MONO_INCLUDE_DIR
    NAMES mono/jit/jit.h
    PATHS
        /usr/include/mono-2.0
        /usr/local/include/mono-2.0
        /Library/Frameworks/Mono.framework/Versions/Current/include
        C:/Program Files/Mono/include
        C:/Program Files (x86)/Mono/include
        ${MONO_ROOT}/include
)

find_library(MONO_LIBRARY
    NAMES mono-2.0
    PATHS
        /usr/lib
        /usr/local/lib
        /Library/Frameworks/Mono.framework/Versions/Current/lib
        C:/Program Files/Mono/lib
        C:/Program Files (x86)/Mono/lib
        ${MONO_ROOT}/lib
)

find_library(MONO_POSIX_LIBRARY
    NAMES monosgen-2.0
    PATHS
        /usr/lib
        /usr/local/lib
        ${MONO_ROOT}/lib
)

# 处理REQUIRED和QUIET参数
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mono
    REQUIRED_VARS MONO_LIBRARY MONO_INCLUDE_DIR
    VERSION_VAR MONO_VERSION
)

if(MONO_FOUND)
    # 创建导入目标
    if(NOT TARGET Mono::Mono)
        add_library(Mono::Mono UNKNOWN IMPORTED)
        set_target_properties(Mono::Mono PROPERTIES
            IMPORTED_LOCATION "${MONO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MONO_INCLUDE_DIR}"
        )

        if(MONO_POSIX_LIBRARY)
            set_target_properties(Mono::Mono PROPERTIES
                INTERFACE_LINK_LIBRARIES "${MONO_POSIX_LIBRARY}"
            )
        endif()

        # 在Windows上添加额外依赖
        if(WIN32)
            find_library(MONO_WIN32_LIBRARY
                NAMES mono-2.0-win32
                PATHS
                    C:/Program Files/Mono/lib
                    C:/Program Files (x86)/Mono/lib
                    ${MONO_ROOT}/lib
            )

            if(MONO_WIN32_LIBRARY)
                set_target_properties(Mono::Mono PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${MONO_WIN32_LIBRARY};${MONO_POSIX_LIBRARY}"
                )
            endif()
        endif()
    endif()

    # 设置变量
    set(MONO_LIBRARIES ${MONO_LIBRARY})
    if(MONO_POSIX_LIBRARY)
        list(APPEND MONO_LIBRARIES ${MONO_POSIX_LIBRARY})
    endif()
    set(MONO_INCLUDE_DIRS ${MONO_INCLUDE_DIR})

    # 获取版本
    if(EXISTS "${MONO_INCLUDE_DIR}/mono/jit/jit.h")
        file(READ "${MONO_INCLUDE_DIR}/mono/jit/jit.h" MONO_JIT_H)
        string(REGEX MATCH "MONO_VERSION[ \t]+\"([0-9]+\\.[0-9]+(\\.[0-9]+)?)\"" MONO_VERSION_MATCH ${MONO_JIT_H})
        if(MONO_VERSION_MATCH)
            set(MONO_VERSION "${CMAKE_MATCH_1}")
        endif()
    endif()

    message(STATUS "Found Mono: ${MONO_LIBRARY} (version ${MONO_VERSION})")
else()
    message(STATUS "Mono not found")
endif()

mark_as_advanced(MONO_INCLUDE_DIR MONO_LIBRARY MONO_POSIX_LIBRARY)