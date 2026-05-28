#include <gtest/gtest.h>
#include <glaze/json/generic.hpp>
#include "resource/ArchiveJson.h"
#include "serialization/Serializable.h"
#include "serialization/SerializationVersion.h"

namespace Prisma::Serialization {

// ============================================================
// 基本类型写入 → 验证 JSON 结构
// ============================================================

TEST(ArchiveJsonTest, WriteFloat) {
    JsonOutputArchive archive;
    archive.BeginObject("root");
    archive.Write("pi", 3.14159f);
    archive.EndObject();

    const json& root = archive.GetJson().get_object().at("root");
    EXPECT_FLOAT_EQ(*root.get_object().at("pi").get_if<double>(), 3.14159);
}

TEST(ArchiveJsonTest, WriteInt32) {
    JsonOutputArchive archive;
    archive.BeginObject("root");
    archive.Write("answer", int32_t(42));
    archive.EndObject();

    const json& root = archive.GetJson().get_object().at("root");
    EXPECT_EQ(static_cast<int32_t>(*root.get_object().at("answer").get_if<double>()), 42);
}

TEST(ArchiveJsonTest, WriteUint32) {
    JsonOutputArchive archive;
    archive.BeginObject("root");
    archive.Write("count", uint32_t(100));
    archive.EndObject();

    const json& root = archive.GetJson().get_object().at("root");
    EXPECT_EQ(static_cast<uint32_t>(*root.get_object().at("count").get_if<double>()), 100u);
}

TEST(ArchiveJsonTest, WriteUint64) {
    JsonOutputArchive archive;
    archive.BeginObject("root");
    archive.Write("big", uint64_t(1234567890123ULL));
    archive.EndObject();

    const json& root = archive.GetJson().get_object().at("root");
    EXPECT_EQ(static_cast<uint64_t>(*root.get_object().at("big").get_if<double>()), 1234567890123ULL);
}

TEST(ArchiveJsonTest, WriteBool) {
    JsonOutputArchive archive;
    archive.BeginObject("root");
    archive.Write("flag", true);
    archive.EndObject();

    const json& root = archive.GetJson().get_object().at("root");
    EXPECT_TRUE(root.get_object().at("flag").get_boolean());
}

TEST(ArchiveJsonTest, WriteString) {
    JsonOutputArchive archive;
    archive.BeginObject("root");
    archive.Write("name", std::string("PrismaEngine"));
    archive.EndObject();

    const json& root = archive.GetJson().get_object().at("root");
    EXPECT_EQ(root.get_object().at("name").get_string(), "PrismaEngine");
}

// ============================================================
// 嵌套对象
// ============================================================

TEST(ArchiveJsonTest, NestedObjectRoundTrip) {
    JsonOutputArchive out;
    out.BeginObject("outer");
    out.Write("level", int32_t(1));
    out.BeginObject("inner");
    out.Write("value", int32_t(99));
    out.EndObject();
    out.EndObject();

    // 验证输出 JSON 结构
    const json& outer = out.GetJson().get_object().at("outer");
    EXPECT_EQ(static_cast<int32_t>(*outer.get_object().at("level").get_if<double>()), 1);

    // 用 InputArchive 读取嵌套对象
    JsonInputArchive in(outer.get_object().at("inner"));
    int32_t val = 0;
    EXPECT_TRUE(in.Read("value", val));
    EXPECT_EQ(val, 99);
}

// ============================================================
// 数学类型 (vec2 / vec3 / vec4 / quat)
// ============================================================

TEST(ArchiveJsonTest, Vec2RoundTrip) {
    JsonOutputArchive out;
    out.BeginObject("v");
    out.Write("pos", PrismaMath::vec2(1.5f, 2.5f));
    out.EndObject();

    JsonInputArchive in(out.GetJson().get_object().at("v"));
    PrismaMath::vec2 val;
    EXPECT_TRUE(in.Read("pos", val));
    EXPECT_FLOAT_EQ(val.x, 1.5f);
    EXPECT_FLOAT_EQ(val.y, 2.5f);
}

TEST(ArchiveJsonTest, Vec3RoundTrip) {
    JsonOutputArchive out;
    out.BeginObject("v");
    out.Write("pos", PrismaMath::vec3(1.0f, 2.0f, 3.0f));
    out.EndObject();

    JsonInputArchive in(out.GetJson().get_object().at("v"));
    PrismaMath::vec3 val;
    EXPECT_TRUE(in.Read("pos", val));
    EXPECT_FLOAT_EQ(val.x, 1.0f);
    EXPECT_FLOAT_EQ(val.y, 2.0f);
    EXPECT_FLOAT_EQ(val.z, 3.0f);
}

TEST(ArchiveJsonTest, Vec4RoundTrip) {
    JsonOutputArchive out;
    out.BeginObject("v");
    out.Write("color", PrismaMath::vec4(0.1f, 0.2f, 0.3f, 1.0f));
    out.EndObject();

    JsonInputArchive in(out.GetJson().get_object().at("v"));
    PrismaMath::vec4 val;
    EXPECT_TRUE(in.Read("color", val));
    EXPECT_FLOAT_EQ(val.x, 0.1f);
    EXPECT_FLOAT_EQ(val.y, 0.2f);
    EXPECT_FLOAT_EQ(val.z, 0.3f);
    EXPECT_FLOAT_EQ(val.w, 1.0f);
}

TEST(ArchiveJsonTest, QuatRoundTrip) {
    PrismaMath::quat q_in(1.0f, 0.0f, 0.0f, 0.0f);
    JsonOutputArchive out;
    out.BeginObject("q");
    out.Write("rot", q_in);
    out.EndObject();

    JsonInputArchive in(out.GetJson().get_object().at("q"));
    PrismaMath::quat val;
    EXPECT_TRUE(in.Read("rot", val));
    EXPECT_FLOAT_EQ(val.w, 1.0f);
    EXPECT_FLOAT_EQ(val.x, 0.0f);
    EXPECT_FLOAT_EQ(val.y, 0.0f);
    EXPECT_FLOAT_EQ(val.z, 0.0f);
}

// ============================================================
// 读写往返 (Round-trip) — 所有基础类型
// ============================================================

TEST(ArchiveJsonTest, FullRoundTrip) {
    JsonOutputArchive out;
    out.BeginObject("data");
    out.Write("f", 3.14f);
    out.Write("i", int32_t(-42));
    out.Write("u", uint32_t(100));
    out.Write("big", uint64_t(1234567890123ULL));
    out.Write("b", true);
    out.Write("s", std::string("hello world"));
    out.EndObject();

    JsonInputArchive in(out.GetJson().get_object().at("data"));

    float f = 0;
    int32_t i = 0;
    uint32_t u = 0;
    uint64_t big = 0;
    bool b = false;
    std::string s;

    EXPECT_TRUE(in.Read("f", f));
    EXPECT_FLOAT_EQ(f, 3.14f);
    EXPECT_TRUE(in.Read("i", i));
    EXPECT_EQ(i, -42);
    EXPECT_TRUE(in.Read("u", u));
    EXPECT_EQ(u, 100u);
    EXPECT_TRUE(in.Read("big", big));
    EXPECT_EQ(big, 1234567890123ULL);
    EXPECT_TRUE(in.Read("b", b));
    EXPECT_TRUE(b);
    EXPECT_TRUE(in.Read("s", s));
    EXPECT_EQ(s, "hello world");
}

// ============================================================
// 序列化版本号兼容性
// ============================================================

TEST(ArchiveJsonTest, DefaultVersion) {
    SerializationVersion v;
    EXPECT_EQ(v.major, 1u);
    EXPECT_EQ(v.minor, 0u);
    EXPECT_EQ(v.patch, 0u);
    EXPECT_EQ(v.ToString(), "1.0.0");
}

TEST(ArchiveJsonTest, VersionFromString) {
    auto v = SerializationVersion::FromString("2.1.3");
    EXPECT_EQ(v.major, 2u);
    EXPECT_EQ(v.minor, 1u);
    EXPECT_EQ(v.patch, 3u);
}

TEST(ArchiveJsonTest, VersionRoundTrip) {
    auto v1 = SerializationVersion::FromString("5.10.255");
    std::string str = v1.ToString();
    auto v2 = SerializationVersion::FromString(str);
    EXPECT_EQ(v1.major, v2.major);
    EXPECT_EQ(v1.minor, v2.minor);
    EXPECT_EQ(v1.patch, v2.patch);
}

// ============================================================
// 错误处理 — 缺失键 / 类型不匹配
// ============================================================

TEST(ArchiveJsonTest, MissingKeyReturnsFalse) {
    json obj = json::object_t{};
    obj.get_object()["present"] = 42.0;

    JsonInputArchive in(obj);
    float val = 0;
    EXPECT_FALSE(in.Read("missing", val));
    EXPECT_TRUE(in.Read("present", val));
    EXPECT_FLOAT_EQ(val, 42.0f);
}

TEST(ArchiveJsonTest, TypeMismatchStringAsFloat) {
    JsonOutputArchive out;
    out.BeginObject("d");
    out.Write("msg", std::string("hello"));
    out.EndObject();

    JsonInputArchive in(out.GetJson().get_object().at("d"));
    float val = 0;
    // 读取字符串键为 float → 抛出 std::bad_variant_access (get_number 内部)
    EXPECT_THROW(in.Read("msg", val), std::bad_variant_access);
}

} // namespace Prisma::Serialization
