#include <gtest/gtest.h>
#include "ai/Blackboard.h"

namespace Prisma {
namespace AI {
namespace {

// ═══════════════════════════════════════════════════════════════════
// 读写操作
// ═══════════════════════════════════════════════════════════════════

TEST(Blackboard_SetGet, SetAndGetInt) {
    Blackboard bb;
    bb.Set<int>("health", 100);
    EXPECT_EQ(bb.Get<int>("health"), 100);
}

TEST(Blackboard_SetGet, SetAndGetFloat) {
    Blackboard bb;
    bb.Set<float>("speed", 12.5f);
    EXPECT_FLOAT_EQ(bb.Get<float>("speed"), 12.5f);
}

TEST(Blackboard_SetGet, SetAndGetString) {
    Blackboard bb;
    bb.Set<std::string>("name", "Player");
    EXPECT_EQ(bb.Get<std::string>("name"), "Player");
}

TEST(Blackboard_SetGet, SetAndGetBool) {
    Blackboard bb;
    bb.Set<bool>("isAlive", true);
    EXPECT_TRUE(bb.Get<bool>("isAlive"));

    bb.Set<bool>("isAlive", false);
    EXPECT_FALSE(bb.Get<bool>("isAlive"));
}

TEST(Blackboard_SetGet, SetAndGetDouble) {
    Blackboard bb;
    bb.Set<double>("precision", 3.14159265358979);
    EXPECT_DOUBLE_EQ(bb.Get<double>("precision"), 3.14159265358979);
}

TEST(Blackboard_SetGet, OverwriteExistingKey) {
    Blackboard bb;
    bb.Set<int>("score", 10);
    EXPECT_EQ(bb.Get<int>("score"), 10);

    bb.Set<int>("score", 50);
    EXPECT_EQ(bb.Get<int>("score"), 50);
}

TEST(Blackboard_SetGet, MultipleKeys_IndependentStorage) {
    Blackboard bb;
    bb.Set<int>("health", 100);
    bb.Set<float>("speed", 5.0f);
    bb.Set<std::string>("name", "Hero");

    EXPECT_EQ(bb.Get<int>("health"), 100);
    EXPECT_FLOAT_EQ(bb.Get<float>("speed"), 5.0f);
    EXPECT_EQ(bb.Get<std::string>("name"), "Hero");
}

TEST(Blackboard_SetGet, LvalueAndRvalueSet) {
    Blackboard bb;

    // 左值
    int val = 42;
    bb.Set<int>("answer", val);
    EXPECT_EQ(bb.Get<int>("answer"), 42);

    // 右值
    bb.Set<int>("temp", 99);
    EXPECT_EQ(bb.Get<int>("temp"), 99);
}

// ═══════════════════════════════════════════════════════════════════
// 删除操作
// ═══════════════════════════════════════════════════════════════════

TEST(Blackboard_Remove, Erase_ExistingKey_ReturnsTrue) {
    Blackboard bb;
    bb.Set<int>("key", 1);
    EXPECT_TRUE(bb.Erase("key"));
}

TEST(Blackboard_Remove, Erase_NonExistentKey_ReturnsFalse) {
    Blackboard bb;
    EXPECT_FALSE(bb.Erase("nonExistent"));
}

TEST(Blackboard_Remove, Erase_RemovesValue) {
    Blackboard bb;
    bb.Set<int>("key", 42);
    bb.Erase("key");
    EXPECT_FALSE(bb.Has("key"));
}

TEST(Blackboard_Remove, Clear_RemovesAllKeys) {
    Blackboard bb;
    bb.Set<int>("a", 1);
    bb.Set<int>("b", 2);
    bb.Set<int>("c", 3);
    EXPECT_EQ(bb.Size(), 3);

    bb.Clear();
    EXPECT_EQ(bb.Size(), 0);
    EXPECT_TRUE(bb.IsEmpty());
}

TEST(Blackboard_Remove, Erase_Twice_SecondReturnsFalse) {
    Blackboard bb;
    bb.Set<int>("key", 1);
    EXPECT_TRUE(bb.Erase("key"));
    EXPECT_FALSE(bb.Erase("key"));
}

// ═══════════════════════════════════════════════════════════════════
// Has / Size / IsEmpty 查询
// ═══════════════════════════════════════════════════════════════════

TEST(Blackboard_Query, Has_ReturnsCorrectValue) {
    Blackboard bb;
    EXPECT_FALSE(bb.Has("anything"));
    bb.Set<int>("anything", 0);
    EXPECT_TRUE(bb.Has("anything"));
}

TEST(Blackboard_Query, Size_ReturnsCorrectCount) {
    Blackboard bb;
    EXPECT_EQ(bb.Size(), 0);
    bb.Set<int>("a", 1);
    EXPECT_EQ(bb.Size(), 1);
    bb.Set<int>("b", 2);
    EXPECT_EQ(bb.Size(), 2);
}

TEST(Blackboard_Query, IsEmpty_InitiallyTrue) {
    Blackboard bb;
    EXPECT_TRUE(bb.IsEmpty());
}

TEST(Blackboard_Query, IsEmpty_AfterSet_False) {
    Blackboard bb;
    bb.Set<int>("x", 1);
    EXPECT_FALSE(bb.IsEmpty());
}

TEST(Blackboard_Query, IsEmpty_AfterClear_True) {
    Blackboard bb;
    bb.Set<int>("x", 1);
    bb.Clear();
    EXPECT_TRUE(bb.IsEmpty());
}

// ═══════════════════════════════════════════════════════════════════
// 类型安全 — 类型不匹配时抛出异常
// ═══════════════════════════════════════════════════════════════════

TEST(Blackboard_TypeSafety, GetWrongType_ThrowsRuntimeError) {
    Blackboard bb;
    bb.Set<int>("value", 42);

    EXPECT_THROW(bb.Get<float>("value"), std::runtime_error);
}

TEST(Blackboard_TypeSafety, GetStringAsInt_ThrowsRuntimeError) {
    Blackboard bb;
    bb.Set<std::string>("text", "hello");

    EXPECT_THROW(bb.Get<int>("text"), std::runtime_error);
}

TEST(Blackboard_TypeSafety, GetBoolAsInt_ThrowsRuntimeError) {
    Blackboard bb;
    bb.Set<bool>("flag", true);

    EXPECT_THROW(bb.Get<int>("flag"), std::runtime_error);
}

// ═══════════════════════════════════════════════════════════════════
// 非存在键处理 — 抛出异常
// ═══════════════════════════════════════════════════════════════════

TEST(Blackboard_NonExistent, Get_ThrowsOutOfRange) {
    Blackboard bb;
    EXPECT_THROW(bb.Get<int>("nonExistent"), std::out_of_range);
}

TEST(Blackboard_NonExistent, GetDouble_ThrowsOutOfRange) {
    Blackboard bb;
    EXPECT_THROW(bb.Get<double>("missing"), std::out_of_range);
}

TEST(Blackboard_NonExistent, GetString_ThrowsOutOfRange) {
    Blackboard bb;
    EXPECT_THROW(bb.Get<std::string>("missing"), std::out_of_range);
}

TEST(Blackboard_NonExistent, Has_ReturnsFalse) {
    Blackboard bb;
    EXPECT_FALSE(bb.Has("nothing"));
}

// ═══════════════════════════════════════════════════════════════════
// TryGet / TryGetValue / GetOrDefault 安全读取
// ═══════════════════════════════════════════════════════════════════

TEST(Blackboard_SafeGet, TryGet_ExistingKey_ReturnsPointer) {
    Blackboard bb;
    bb.Set<int>("score", 100);

    int* ptr = bb.TryGet<int>("score");
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 100);
}

TEST(Blackboard_SafeGet, TryGet_NonExistentKey_ReturnsNullptr) {
    Blackboard bb;
    EXPECT_EQ(bb.TryGet<int>("nothing"), nullptr);
}

TEST(Blackboard_SafeGet, TryGet_WrongType_ReturnsNullptr) {
    Blackboard bb;
    bb.Set<int>("value", 42);

    EXPECT_EQ(bb.TryGet<float>("value"), nullptr);
}

TEST(Blackboard_SafeGet, TryGetValue_ExistingKey_ReturnsValue) {
    Blackboard bb;
    bb.Set<int>("level", 5);

    EXPECT_EQ(bb.TryGetValue<int>("level"), 5);
}

TEST(Blackboard_SafeGet, TryGetValue_NonExistentKey_ReturnsDefault) {
    Blackboard bb;
    EXPECT_EQ(bb.TryGetValue<int>("missing", -1), -1);
    EXPECT_EQ(bb.TryGetValue<float>("missing", 0.0f), 0.0f);
    EXPECT_EQ(bb.TryGetValue<std::string>("missing", "fallback"), "fallback");
}

TEST(Blackboard_SafeGet, TryGetValue_WrongType_ReturnsDefault) {
    Blackboard bb;
    bb.Set<int>("value", 42);

    EXPECT_EQ(bb.TryGetValue<float>("value", -1.0f), -1.0f);
}

TEST(Blackboard_SafeGet, GetOrDefault_ExistingKey_ReturnsValue) {
    Blackboard bb;
    bb.Set<int>("count", 10);

    EXPECT_EQ(bb.GetOrDefault<int>("count", 0), 10);
}

TEST(Blackboard_SafeGet, GetOrDefault_NonExistentKey_ReturnsDefault) {
    Blackboard bb;
    EXPECT_EQ(bb.GetOrDefault<int>("missing", 99), 99);
    EXPECT_EQ(bb.GetOrDefault<std::string>("missing", "none"), "none");
}

TEST(Blackboard_SafeGet, GetOrDefault_WrongType_ReturnsDefault) {
    Blackboard bb;
    bb.Set<int>("value", 42);

    EXPECT_EQ(bb.GetOrDefault<float>("value", 1.0f), 1.0f);
}

// ═══════════════════════════════════════════════════════════════════
// 移动语义
// ═══════════════════════════════════════════════════════════════════

TEST(Blackboard_Move, MoveConstructor_TransfersData) {
    Blackboard src;
    src.Set<int>("key", 42);

    Blackboard dst(std::move(src));
    EXPECT_EQ(dst.Get<int>("key"), 42);
}

TEST(Blackboard_Move, MoveAssigment_TransfersData) {
    Blackboard src;
    src.Set<int>("key", 42);

    Blackboard dst;
    dst = std::move(src);
    EXPECT_EQ(dst.Get<int>("key"), 42);
}

TEST(Blackboard_Move, AfterMove_SourceIsEmpty) {
    Blackboard src;
    src.Set<int>("key", 42);

    Blackboard dst(std::move(src));

    // 移动后的源对象处于有效但未指定状态
    // 再次使用是安全的
    EXPECT_TRUE(src.IsEmpty());
}

// ═══════════════════════════════════════════════════════════════════
// 复杂类型
// ═══════════════════════════════════════════════════════════════════

TEST(Blackboard_ComplexTypes, VectorOfInts) {
    Blackboard bb;
    std::vector<int> vec = {1, 2, 3, 4, 5};
    bb.Set<std::vector<int>>("data", vec);

    auto result = bb.Get<std::vector<int>>("data");
    ASSERT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[4], 5);
}

TEST(Blackboard_ComplexTypes, MapOfStrings) {
    Blackboard bb;
    std::unordered_map<std::string, std::string> map;
    map["hello"] = "world";
    bb.Set<std::unordered_map<std::string, std::string>>("dict", map);

    auto result = bb.Get<std::unordered_map<std::string, std::string>>("dict");
    EXPECT_EQ(result["hello"], "world");
}

} // namespace
} // namespace AI
} // namespace Prisma
