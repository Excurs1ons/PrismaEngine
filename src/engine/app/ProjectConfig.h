#pragma once

#include "Export.h"
#include "graphic/interfaces/RenderTypes.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include <glaze/glaze.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace Prisma {

/* C# 脚本后端枚举（与编译时 PRISMA_ENABLE_SCRIPTING 值对应） */
enum class RenderMode : uint8_t {
    Mode2D = 0,
    Mode3D_Forward = 1,
    Mode3D_ForwardPlus = 2,
    Mode3D_Deferred = 3,
    Mode3D_DeferredPlus = 4,
    Mode3D_PathTracing = 5,
    Mode3D_ClusteredForward = 6,
    SRP = 7
};

enum class ScriptingBackend : uint8_t {
    Off     = 0,
    Mono    = 1,
    CoreCLR = 2
};

struct WindowConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool fullscreen = true;
    bool resizable = true;
    Graphic::PresentMode vsync = Graphic::PresentMode::Mailbox;
    uint32_t maxFPS = 0;
};

struct RenderingConfig {
    uint32_t maxSamples = 512;
    uint32_t maxBounces = 8;
    bool hardwareRayTracing = false; // DXR 硬件加速预留
    Graphic::PathTraceMode pathTraceMode = Graphic::PathTraceMode::BVH;
    bool enableNEE = false;            // 下一事件估计（小光源时显著提升收敛）
};

struct HeadlessConfig {
    uint32_t frames = 500;
    uint32_t width = 1080;
    uint32_t height = 1080;
    std::string outputPath = "pt_output.png";
};

struct ProjectConfig {
    std::string name = "Prisma App";
    std::string entryScene;
    std::vector<std::string> assets;
    std::vector<std::string> scenes;   // 可选的多场景列表（F6/F7 切换）
    WindowConfig window;
    RenderMode renderMode = RenderMode::Mode3D_Forward;
    ScriptingBackend scriptingBackend = ScriptingBackend::CoreCLR;
    RenderingConfig rendering;
    HeadlessConfig headless;
};

} // namespace Prisma

// Glaze 映射

template <>
struct glz::meta<Prisma::RenderMode> {
    using enum Prisma::RenderMode;
    static constexpr auto value = glz::enumerate(
        "2D", Mode2D,
        "Forward", Mode3D_Forward,
        "Forward+", Mode3D_ForwardPlus,
        "Deferred", Mode3D_Deferred,
        "Deferred+", Mode3D_DeferredPlus,
        "PathTracing", Mode3D_PathTracing,
        "ClusteredForward", Mode3D_ClusteredForward,
        "SRP", SRP
    );
};

template <>
struct glz::meta<Prisma::ScriptingBackend> {
    using enum Prisma::ScriptingBackend;
    static constexpr auto value = glz::enumerate(
        "Off", Off,
        "Mono", Mono,
        "CoreCLR", CoreCLR
    );
};

template <>
struct glz::meta<Prisma::Graphic::PresentMode> {
    using enum Prisma::Graphic::PresentMode;
    static constexpr auto value = glz::enumerate(
        "Immediate", Immediate,
        "Mailbox", Mailbox,
        "Adaptive", Adaptive,
        "VSync", VSync
    );
};

template <>
struct glz::meta<Prisma::Graphic::PathTraceMode> {
    using enum Prisma::Graphic::PathTraceMode;
    static constexpr auto value = glz::enumerate(
        "Flat", Flat,
        "BVH", BVH,
        "HardwareRT", HardwareRT
    );
};

template <>
struct glz::meta<Prisma::WindowConfig> {
    static constexpr auto value = glz::object(
        "width", &Prisma::WindowConfig::width,
        "height", &Prisma::WindowConfig::height,
        "fullscreen", &Prisma::WindowConfig::fullscreen,
        "resizable", &Prisma::WindowConfig::resizable,
        "vsync", &Prisma::WindowConfig::vsync,
        "maxFPS", &Prisma::WindowConfig::maxFPS
    );
};

template <>
struct glz::meta<Prisma::RenderingConfig> {
    static constexpr auto value = glz::object(
        "maxSamples", &Prisma::RenderingConfig::maxSamples,
        "maxBounces", &Prisma::RenderingConfig::maxBounces,
        "hardwareRayTracing", &Prisma::RenderingConfig::hardwareRayTracing,
        "pathTraceMode", &Prisma::RenderingConfig::pathTraceMode,
        "enableNEE", &Prisma::RenderingConfig::enableNEE
    );
};

template <>
struct glz::meta<Prisma::HeadlessConfig> {
    static constexpr auto value = glz::object(
        "frames",     &Prisma::HeadlessConfig::frames,
        "width",      &Prisma::HeadlessConfig::width,
        "height",     &Prisma::HeadlessConfig::height,
        "outputPath", &Prisma::HeadlessConfig::outputPath
    );
};

template <>
struct glz::meta<Prisma::ProjectConfig> {
    static constexpr auto value = glz::object(
        "name", &Prisma::ProjectConfig::name,
        "entryScene", &Prisma::ProjectConfig::entryScene,
        "assets", &Prisma::ProjectConfig::assets,
        "scenes", &Prisma::ProjectConfig::scenes,
        "window", &Prisma::ProjectConfig::window,
        "renderMode", &Prisma::ProjectConfig::renderMode,
        "scriptingBackend", &Prisma::ProjectConfig::scriptingBackend,
        "rendering", &Prisma::ProjectConfig::rendering,
        "headless", &Prisma::ProjectConfig::headless
    );
};
