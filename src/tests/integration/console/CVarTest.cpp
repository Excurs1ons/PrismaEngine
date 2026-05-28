#include <gtest/gtest.h>
#include "console/CVar.h"

namespace Prisma {
namespace {

TEST(CVarTest, IntCVar) {
    CVar<int> cvar("test_int", 42, "An integer CVar");
    EXPECT_EQ(cvar.GetName(), "test_int");
    EXPECT_EQ(cvar.GetDescription(), "An integer CVar");
    EXPECT_EQ(cvar.GetTypeName(), "int");
    EXPECT_EQ(cvar.Get(), 42);
    EXPECT_EQ(static_cast<int>(cvar), 42);

    cvar.Set(100);
    EXPECT_EQ(cvar.Get(), 100);
}

TEST(CVarTest, FloatCVar) {
    CVar<float> cvar("test_float", 1.0f, 0.0f, 1.0f, "A float CVar");
    EXPECT_EQ(cvar.GetTypeName(), "float");
    EXPECT_FLOAT_EQ(cvar.Get(), 1.0f);

    cvar.Set(0.5f);
    EXPECT_FLOAT_EQ(cvar.Get(), 0.5f);
}

TEST(CVarTest, FloatCVarClamping) {
    CVar<float> cvar("clamped", 0.5f, 0.0f, 1.0f);
    cvar.Set(2.0f);
    EXPECT_FLOAT_EQ(cvar.Get(), 1.0f);

    cvar.Set(-1.0f);
    EXPECT_FLOAT_EQ(cvar.Get(), 0.0f);
}

TEST(CVarTest, BoolCVar) {
    CVar<bool> cvar("test_bool", false);
    EXPECT_EQ(cvar.GetTypeName(), "bool");
    EXPECT_FALSE(cvar.Get());

    cvar.Set(true);
    EXPECT_TRUE(cvar.Get());
}

TEST(CVarTest, BoolCVarParsing) {
    CVar<bool> cvar("test_bool", false);
    cvar.SetFromString("true");
    EXPECT_TRUE(cvar.Get());

    cvar.SetFromString("false");
    EXPECT_FALSE(cvar.Get());

    cvar.SetFromString("1");
    EXPECT_TRUE(cvar.Get());

    cvar.SetFromString("0");
    EXPECT_FALSE(cvar.Get());

    cvar.SetFromString("on");
    EXPECT_TRUE(cvar.Get());
}

TEST(CVarTest, StringCVar) {
    CVar<std::string> cvar("test_str", "hello");
    EXPECT_EQ(cvar.GetTypeName(), "string");
    EXPECT_EQ(cvar.Get(), "hello");

    cvar.Set("world");
    EXPECT_EQ(cvar.Get(), "world");
}

TEST(CVarTest, SetFromString) {
    CVar<int> cvar("from_str", 0);
    cvar.SetFromString("42");
    EXPECT_EQ(cvar.Get(), 42);
}

TEST(CVarTest, GetString) {
    CVar<int> cvar("to_str", 99);
    EXPECT_EQ(cvar.GetString(), "99");
}

TEST(CVarTest, ResetToDefault) {
    CVar<int> cvar("reset", 10, "resets to 10");
    cvar.Set(99);
    EXPECT_EQ(cvar.Get(), 99);

    cvar.ResetToDefault();
    EXPECT_EQ(cvar.Get(), 10);
}

TEST(CVarTest, ReadOnlyFlagPreventsModification) {
    CVar<int> cvar("readonly", 5, "", CVarFlags::ReadOnly);
    EXPECT_TRUE(cvar.HasFlag(CVarFlags::ReadOnly));

    cvar.Set(100);
    EXPECT_EQ(cvar.Get(), 5);

    cvar.SetFromString("200");
    EXPECT_EQ(cvar.Get(), 5);
}

TEST(CVarTest, CVarFlagsOperators) {
    auto combined = CVarFlags::Cheat | CVarFlags::ReadOnly;
    EXPECT_TRUE(HasFlag(combined, CVarFlags::Cheat));
    EXPECT_TRUE(HasFlag(combined, CVarFlags::ReadOnly));
    EXPECT_FALSE(HasFlag(combined, CVarFlags::Archive));
}

TEST(CVarTest, CVarRegistry) {
    CVarRegistry registry;

    registry.Register(std::make_unique<CVar<int>>("fps_max", 60));
    registry.Register(std::make_unique<CVar<float>>("volume", 1.0f, 0.0f, 1.0f));
    registry.Register(std::make_unique<CVar<bool>>("vsync", true));

    EXPECT_EQ(registry.GetCount(), 3u);

    auto* cvar = registry.Find("fps_max");
    ASSERT_NE(cvar, nullptr);
    EXPECT_EQ(cvar->GetName(), "fps_max");
    EXPECT_EQ(cvar->GetTypeName(), "int");
    EXPECT_EQ(cvar->GetString(), "60");

    EXPECT_EQ(registry.Find("nonexistent"), nullptr);
}

TEST(CVarTest, CVarRegistryForEach) {
    CVarRegistry registry;
    registry.Register(std::make_unique<CVar<int>>("a", 1));
    registry.Register(std::make_unique<CVar<int>>("b", 2));
    registry.Register(std::make_unique<CVar<int>>("c", 3));

    int count = 0;
    registry.ForEach([&count](CVarBase*) { count++; });
    EXPECT_EQ(count, 3);
}

TEST(CVarTest, CVarRegistryGetAll) {
    CVarRegistry registry;
    registry.Register(std::make_unique<CVar<int>>("only_one", 1));

    auto all = registry.GetAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0]->GetName(), "only_one");
}

TEST(CVarTest, IntCVarWithMinMax) {
    CVar<int> cvar("bounded", 50, 0, 100);
    EXPECT_TRUE(cvar.GetMin().has_value());
    EXPECT_TRUE(cvar.GetMax().has_value());
    EXPECT_EQ(cvar.GetMin().value(), 0);
    EXPECT_EQ(cvar.GetMax().value(), 100);

    cvar.Set(-10);
    EXPECT_EQ(cvar.Get(), 0);

    cvar.Set(200);
    EXPECT_EQ(cvar.Get(), 100);
}

}
} // namespace Prisma
