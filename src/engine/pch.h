#pragma once

#ifdef _WIN32
#ifndef NOGDI
#define NOGDI
#endif
#include <windows.h>
#include <windowsx.h>
#pragma push_macro("FindResource")
#ifdef FindResource
#undef FindResource
#endif
#pragma pop_macro("FindResource")
#undef FindResource
#endif

#include <algorithm>
#include <utility>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <filesystem>

#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include <vulkan/vulkan.h>
#endif

#ifdef PRISMA_ENABLE_AUDIO_SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif

#ifdef PRISMA_BUILD_EDITOR
#include <imgui.h>
#endif

#ifdef PRISMA_USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) \
    { \
        if (p) { \
            delete (p); \
            (p) = nullptr; \
        } \
    }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) \
    { \
        if (p) { \
            delete[] (p); \
            (p) = nullptr; \
        } \
    }
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif

#ifdef _DEBUG
#define DEBUG_ONLY(x) x
#define RELEASE_ONLY(x)
#else
#define DEBUG_ONLY(x)
#define RELEASE_ONLY(x) x
#endif

using byte   = unsigned char;
using sbyte  = signed char;
using uint8  = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;
using int8   = signed char;
using int16  = signed short;
using int32  = int;
using int64  = long long;

namespace PrismaEngine::Constants {
constexpr float PI         = 3.14159265358979323846f;
constexpr float TWO_PI     = 2.0f * PI;
constexpr float HALF_PI    = PI * 0.5f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;
}

using namespace PrismaEngine::Constants;

template <typename T> T Clamp(T value, T min, T max) {
    return std::max(min, std::min(max, value));
}

template <typename T> T Lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
