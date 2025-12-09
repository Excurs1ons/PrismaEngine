# 输出 Android 构建环境信息
message(STATUS "=== Android Build Environment Information ===")

# 检查 ANDROID_NDK_HOME
if(DEFINED ANDROID_NDK_HOME)
    message(STATUS "ANDROID_NDK_HOME is defined: ${ANDROID_NDK_HOME}")

    # 检查 NDK 路径是否存在
    if(EXISTS "${ANDROID_NDK_HOME}")
        message(STATUS "NDK path exists: ${ANDROID_NDK_HOME}")

        # 查找 NDK 版本信息
        if(EXISTS "${ANDROID_NDK_HOME}/source.properties")
            file(STRINGS "${ANDROID_NDK_HOME}/source.properties" NDK_PROPERTIES)
            foreach(LINE ${NDK_PROPERTIES})
                if(LINE MATCHES "Pkg\\.Revision")
                    message(STATUS "NDK Version: ${LINE}")
                endif()
            endforeach()
        endif()
    else()
        message(FATAL_ERROR "NDK path does not exist: ${ANDROID_NDK_HOME}")
    endif()
else()
    message(STATUS "ANDROID_NDK_HOME is not defined")
endif()

# 检查 ANDROID_HOME
if(DEFINED ENV{ANDROID_HOME})
    message(STATUS "ANDROID_HOME environment variable: $ENV{ANDROID_HOME}")

    # 检查 SDK 路径是否存在
    if(EXISTS "$ENV{ANDROID_HOME}")
        message(STATUS "SDK path exists: $ENV{ANDROID_HOME}")
    else()
        message(WARNING "SDK path does not exist: $ENV{ANDROID_HOME}")
    endif()
else()
    message(STATUS "ANDROID_HOME environment variable is not set")
endif()

# 检查 ANDROID_SDK_ROOT
if(DEFINED ENV{ANDROID_SDK_ROOT})
    message(STATUS "ANDROID_SDK_ROOT environment variable: $ENV{ANDROID_SDK_ROOT}")
else()
    message(STATUS "ANDROID_SDK_ROOT environment variable is not set")
endif()

# 如果 ANDROID_NDK_HOME 未定义，尝试从 ANDROID_HOME 推导
if(NOT DEFINED ANDROID_NDK_HOME)
    if(DEFINED ENV{ANDROID_HOME})
        message(STATUS "Attempting to find NDK in ANDROID_HOME...")
        file(GLOB NDK_CANDIDATES "$ENV{ANDROID_HOME}/ndk/*")
        if(NDK_CANDIDATES)
            list(SORT NDK_CANDIDATES DESC)
            list(GET NDK_CANDIDATES 0 ANDROID_NDK_HOME)
            message(STATUS "Found NDK: ${ANDROID_NDK_HOME}")
        else()
            message(FATAL_ERROR "No NDK found in ANDROID_HOME: $ENV{ANDROID_HOME}")
        endif()
    endif()
endif()

# 最终检查 NDK 是否可用
if(NOT ANDROID_NDK_HOME)
    message(FATAL_ERROR "No NDK found. Please set ANDROID_NDK_HOME or ANDROID_HOME environment variable.")
endif()

# 输出 Android 构建配置信息
message(STATUS "Android ABI: ${ANDROID_ABI}")
message(STATUS "Android Platform: ${ANDROID_PLATFORM}")
message(STATUS "Android STL: ${ANDROID_STL}")

message(STATUS "=== End Android Build Environment Information ===")
