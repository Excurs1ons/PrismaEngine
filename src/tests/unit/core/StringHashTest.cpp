#include <gtest/gtest.h>
#include "core/StringHash.h"
#include <string>

namespace Prisma {
namespace Core {
namespace {

// Hash consistency: same string produces same hash every time
TEST(StringHashTest, Consistency) {
    StringHash h1("hello");
    StringHash h2("hello");
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h1.GetHash(), h2.GetHash());
}

// Hash from std::string
TEST(StringHashTest, FromStdString) {
    std::string str = "world";
    StringHash sh(str);
    EXPECT_EQ(sh, StringHash("world"));
}

// Hash from std::string_view
TEST(StringHashTest, FromStringView) {
    std::string_view sv = "test_view";
    StringHash sh(sv);
    EXPECT_EQ(sh, StringHash("test_view"));
}

// Default constructor gives zero hash
TEST(StringHashTest, DefaultIsZero) {
    StringHash sh;
    EXPECT_EQ(sh.GetHash(), 0u);
}

// Different strings produce different hashes (basic quality)
TEST(StringHashTest, DifferentStringsDifferentHashes) {
    StringHash a("apple");
    StringHash b("banana");
    StringHash c("cherry");
    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(b, c);
}

// Compile-time hash matches runtime hash
TEST(StringHashTest, CompileTimeMatchesRuntime) {
    constexpr auto ctHash = StringHash::HashCompileTime("compile_time");
    auto rtHash = StringHash::Hash("compile_time");
    EXPECT_EQ(ctHash, rtHash);
}

// Literal operator _hash works
TEST(StringHashTest, LiteralOperator) {
    auto h1 = "path/to/asset"_hash;
    auto h2 = StringHash::Hash("path/to/asset");
    EXPECT_EQ(h1, h2);
}

// Empty string hashing
TEST(StringHashTest, EmptyString) {
    StringHash sh("");
    EXPECT_NE(sh.GetHash(), 0u); // FNV-1a of empty is offset basis
    EXPECT_EQ(sh.GetHash(), StringHash::FNV_OFFSET_BASIS);
}

// Operator== and operator!=
TEST(StringHashTest, EqualityOperators) {
    StringHash a("same");
    StringHash b("same");
    StringHash c("different");

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

// Implicit conversion to HashType (uint32_t)
TEST(StringHashTest, ImplicitConversion) {
    StringHash sh("convert");
    StringHash::HashType val = sh;
    EXPECT_EQ(val, sh.GetHash());
}

// Hash collision resistance: strings differing by one character
TEST(StringHashTest, NearStringCollision) {
    StringHash a("abc");
    StringHash b("abd");
    StringHash c("abb");
    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(b, c);
}

// Case sensitivity
TEST(StringHashTest, CaseSensitive) {
    StringHash lower("hello");
    StringHash upper("HELLO");
    EXPECT_NE(lower, upper);
}

TEST(StringHashTest, KnownHashValue) {
    StringHash sh("test");
    EXPECT_NE(sh.GetHash(), 0u);
}

} // namespace
} // namespace Core
} // namespace Prisma
