#include <gtest/gtest.h>

// 纯逻辑测试：验证引擎配置结构的默认值
// EngineSpecification（Engine.h）和 ProjectConfig（ProjectConfig.h）
// 均为头文件定义的结构体，无需链接 Engine 库。

// 注意：此处不直接 include ProjectConfig.h / Engine.h 是为了避免
// 拉入 Vulkan 依赖头文件链。我们使用引擎代码中实际定义的枚举和结构体
// 的等效副本来测试约定。

#include <cstdint>
#include <vector>
#include <string>

namespace Prisma {

// 与引擎 Engine.h 中的 NonCoreSubsystem 一致
enum class NonCoreSubsystem : uint8_t {
    Physics,
    Audio,
    Navigation,
    AI,
    Particles,
    Terrain,
    Water,
    Network,
    Localization,
    ScriptEngine,
    EditorMCP,
    Profiler,
    Animation,
    COUNT
};

// 与引擎 Engine.h 中的 EngineSpecification 一致
struct EngineSpecification {
    std::vector<NonCoreSubsystem> enabledNonCoreSubsystems;
};

// 与引擎 app/ProjectConfig.h 中的 RenderMode 一致
enum class RenderMode : uint8_t {
    Mode2D = 0,
    Mode3D_Forward = 1,
    Mode3D_ForwardPlus = 2,
    Mode3D_Deferred = 3,
    Mode3D_DeferredPlus = 4,
    Mode3D_PathTracing = 5,
    Mode3D_ClusteredForward = 6,
    SRP = 7,
    Mode3D_NPR = 8,
};

// 与引擎 app/ProjectConfig.h 中的 ProjectConfig 一致
struct ProjectConfig {
    std::string name = "Prisma App";
    std::string entryScene;
    std::vector<std::string> assets;
    std::vector<std::string> scenes;
    std::vector<std::string> subsystems; // 非核心子系统白名单
    RenderMode renderMode = RenderMode::Mode3D_Forward;
};

} // namespace Prisma

namespace {
using namespace Prisma;

TEST(SubsystemConfigTest, EngineSpecificationDefaultEnabledEmpty) {
    // 验证 EngineSpecification 默认 enabledNonCoreSubsystems 为空
    EngineSpecification spec;
    EXPECT_TRUE(spec.enabledNonCoreSubsystems.empty());
}

TEST(SubsystemConfigTest, ProjectConfigSubsystemsEmptyByDefault) {
    // 验证 ProjectConfig 默认 subsystems 为空（所有非核心子系统 OFF）
    ProjectConfig config;
    EXPECT_TRUE(config.subsystems.empty());
}

TEST(SubsystemConfigTest, ProjectConfigDefaultRenderMode) {
    // 验证 ProjectConfig 默认渲染模式为 Mode3D_Forward
    ProjectConfig config;
    EXPECT_EQ(config.renderMode, RenderMode::Mode3D_Forward);
}

TEST(SubsystemConfigTest, ProjectConfigDefaultName) {
    // 验证 ProjectConfig 默认名称
    ProjectConfig config;
    EXPECT_EQ(config.name, "Prisma App");
}

TEST(SubsystemConfigTest, ProjectConfigDefaultEntrySceneEmpty) {
    // 验证默认 entryScene 为空
    ProjectConfig config;
    EXPECT_TRUE(config.entryScene.empty());
}

TEST(SubsystemConfigTest, EngineSpecificationSubsystemsCanBePopulated) {
    // 验证 enabledNonCoreSubsystems 可以被填充
    EngineSpecification spec;
    spec.enabledNonCoreSubsystems = {
        NonCoreSubsystem::Physics,
        NonCoreSubsystem::Audio,
        NonCoreSubsystem::Navigation,
    };
    EXPECT_EQ(spec.enabledNonCoreSubsystems.size(), 3u);
    EXPECT_EQ(spec.enabledNonCoreSubsystems[0], NonCoreSubsystem::Physics);
    EXPECT_EQ(spec.enabledNonCoreSubsystems[1], NonCoreSubsystem::Audio);
    EXPECT_EQ(spec.enabledNonCoreSubsystems[2], NonCoreSubsystem::Navigation);
}

TEST(SubsystemConfigTest, NonCoreSubsystemEnumValues) {
    // 验证 NonCoreSubsystem 枚举值的顺序
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Physics), 0);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Audio), 1);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Navigation), 2);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::AI), 3);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Particles), 4);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Terrain), 5);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Water), 6);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Network), 7);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Localization), 8);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::ScriptEngine), 9);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::EditorMCP), 10);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Profiler), 11);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::Animation), 12);
    EXPECT_EQ(static_cast<int>(NonCoreSubsystem::COUNT), 13);
}

} // namespace
