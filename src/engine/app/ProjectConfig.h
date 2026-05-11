#pragma once

#include "Export.h"
#include "graphic/interfaces/RenderTypes.h"
#include <glaze/glaze.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace Prisma {

/**
 * @brief C# 脚本后端枚举（与编译时 PRISMA_ENABLE_SCRIPTING 值对应）
 *   Off    = 0：关闭 —— 不初始化子系统，不加载 DLL，只使用 Native 逻辑
 *   Mono   = 1：Mono 运行时
 *   CoreCLR = 2：.NET CoreCLR 宿主
 */
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

struct ProjectConfig {
    std::string name = "Prisma App";
    std::string entryScene;
    std::vector<std::string> assets;
    WindowConfig window;
    ScriptingBackend scriptingBackend = ScriptingBackend::CoreCLR;
};

} // namespace Prisma

// ── Glaze 映射 ──

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
struct glz::meta<Prisma::ProjectConfig> {
    static constexpr auto value = glz::object(
        "name", &Prisma::ProjectConfig::name,
        "entryScene", &Prisma::ProjectConfig::entryScene,
        "assets", &Prisma::ProjectConfig::assets,
        "window", &Prisma::ProjectConfig::window,
        "scriptingBackend", &Prisma::ProjectConfig::scriptingBackend
    );
};
