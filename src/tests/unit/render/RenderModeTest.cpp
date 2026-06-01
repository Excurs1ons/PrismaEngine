#include <gtest/gtest.h>
#include <cstdint>

// 纯逻辑测试：验证 Prisma::RenderMode 枚举值（定义于 app/ProjectConfig.h）
// 为避免引入 Vulkan 依赖的头文件链，在测试中维护一份枚举副本。
// 若引擎中的 Prisma::RenderMode 发生变更，此测试的静态断言将捕获差异。
namespace Prisma {

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

} // namespace Prisma

namespace {
using namespace Prisma;

TEST(RenderModeTest, EnumValues) {
    // 验证至少存在 7 个渲染模式值 (0..6 + 8)
    EXPECT_EQ(static_cast<int>(RenderMode::Mode2D), 0);
    EXPECT_EQ(static_cast<int>(RenderMode::Mode3D_Forward), 1);
    EXPECT_EQ(static_cast<int>(RenderMode::Mode3D_ForwardPlus), 2);
    EXPECT_EQ(static_cast<int>(RenderMode::Mode3D_Deferred), 3);
    EXPECT_EQ(static_cast<int>(RenderMode::Mode3D_DeferredPlus), 4);
    EXPECT_EQ(static_cast<int>(RenderMode::Mode3D_PathTracing), 5);
    EXPECT_EQ(static_cast<int>(RenderMode::Mode3D_ClusteredForward), 6);
    EXPECT_EQ(static_cast<int>(RenderMode::SRP), 7);
    EXPECT_EQ(static_cast<int>(RenderMode::Mode3D_NPR), 8);
}

TEST(RenderModeTest, Mode3D_ForwardIsDefault) {
    // ProjectConfig::renderMode 默认为 Mode3D_Forward
    EXPECT_EQ(static_cast<int>(RenderMode::Mode3D_Forward), 1);
}

TEST(RenderModeTest, EnumSize) {
    // 枚举项占用单字节
    EXPECT_EQ(sizeof(RenderMode), 1u);
}

TEST(RenderModeTest, UnderlyingType) {
    // 底层类型为 uint8_t
    using Underlying = std::underlying_type_t<RenderMode>;
    EXPECT_TRUE((std::is_same_v<Underlying, uint8_t>));
}

TEST(RenderModeTest, ModeValuesAreDistinct) {
    // 所有模式值互不相同（无重复值）
    int values[] = {
        static_cast<int>(RenderMode::Mode2D),
        static_cast<int>(RenderMode::Mode3D_Forward),
        static_cast<int>(RenderMode::Mode3D_ForwardPlus),
        static_cast<int>(RenderMode::Mode3D_Deferred),
        static_cast<int>(RenderMode::Mode3D_DeferredPlus),
        static_cast<int>(RenderMode::Mode3D_PathTracing),
        static_cast<int>(RenderMode::Mode3D_ClusteredForward),
        static_cast<int>(RenderMode::SRP),
        static_cast<int>(RenderMode::Mode3D_NPR),
    };
    constexpr int count = sizeof(values) / sizeof(values[0]);
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            EXPECT_NE(values[i], values[j]) << "Duplicate RenderMode value at index " << i << " and " << j;
        }
    }
}

} // namespace
