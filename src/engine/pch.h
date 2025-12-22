#pragma once

// 平台定义
#ifdef _WIN32
    #include <windows.h>
    #include <windowsx.h>
    // DirectX头文件将在需要时包含
    // 不在PCH中包含以避免版本兼容性问题
#endif

// C++标准库核心头文件
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// C标准库头文件
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <ciso646>
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <cwctype>

// 文件系统
#include <filesystem>

// 数学库
#include <cmath>
#include <numbers>

// 异常处理
#include <exception>
#include <stdexcept>
#include <system_error>

// 智能指针（已经在上面，但强调一下）
#include <memory>

// 字符串流
#include <sstream>
#include <iomanip>

// 第三方库
#ifdef ENABLE_VULKAN
    #include <vulkan/vulkan.h>
#endif

#ifdef ENABLE_SDL
    #include <SDL3/SDL.h>
    #include <SDL3/SDL_main.h>
#endif

#ifdef USE_IMGUI
    #include <imgui.h>
#endif

#ifdef USE_NLOHMANN_JSON
    #include <nlohmann/json.hpp>
#endif

// 注意：不要在PCH中包含项目特定的头文件
// 这些头文件会在各自需要时被包含
// 只包含标准库和第三方库头文件

// 常用宏定义（使用条件编译避免重定义）
#ifndef SAFE_DELETE
    #define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }
#endif

#ifndef SAFE_DELETE_ARRAY
    #define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p) = nullptr; } }
#endif

// 注意：SAFE_RELEASE 在 Helper.h 中定义，这里不定义以避免冲突

// 禁用某些警告
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251)  // class 'xxx' needs to have dll-interface
    #pragma warning(disable: 4275)  // non dll-interface class 'xxx' used as base for dll-interface class 'yyy'
#endif

// 便于调试的宏
#ifdef _DEBUG
    #define DEBUG_ONLY(x) x
    #define RELEASE_ONLY(x)
#else
    #define DEBUG_ONLY(x)
    #define RELEASE_ONLY(x) x
#endif

// 常用的类型别名
using byte = unsigned char;
using sbyte = signed char;
using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;
using int8 = signed char;
using int16 = signed short;
using int32 = int;
using int64 = long long;

// 单位转换常量
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// 常用的数学函数
template<typename T>
T Clamp(T value, T min, T max) {
    return std::max(min, std::min(max, value));
}

template<typename T>
T Lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

// 恢复警告设置
#ifdef _MSC_VER
    #pragma warning(pop)
#endif