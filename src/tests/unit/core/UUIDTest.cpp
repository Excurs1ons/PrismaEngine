#include <gtest/gtest.h>
#include "core/UUID.h"
#include <unordered_set>

namespace Prisma {
namespace {

// UUID::create() generates unique values across multiple calls
TEST(UUIDTest, GenerateUniqueValues) {
    UUID a;
    UUID b;
    UUID c;

    uint64_t va = static_cast<uint64_t>(a);
    uint64_t vb = static_cast<uint64_t>(b);
    uint64_t vc = static_cast<uint64_t>(c);

    // All three should be different (astronomically unlikely to collide)
    EXPECT_NE(va, vb);
    EXPECT_NE(va, vc);
    EXPECT_NE(vb, vc);
}

// UUID::ToString() / UUID::FromString() round-trip conversion
TEST(UUIDTest, ToStringFromStringRoundTrip) {
    UUID original;
    std::string str = original.ToString();
    UUID restored = UUID::FromString(str);

    EXPECT_EQ(static_cast<uint64_t>(original), static_cast<uint64_t>(restored));
}

// Hex string format: 16 hex digits, zero-padded
TEST(UUIDTest, ToStringHexFormat) {
    UUID value(0xABCD1234);
    // ToString() outputs: "00000000abcd1234"
    std::string str = value.ToString();
    EXPECT_EQ(str.size(), 16);
    // All characters should be hex digits
    for (char c : str) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

// UUID(uint64_t) constructor preserves exact value
TEST(UUIDTest, ExplicitValueConstructor) {
    uint64_t testValues[] = {0, 1, 0xFFFFFFFFFFFFFFFFULL, 0xDEADBEEF};
    for (auto v : testValues) {
        UUID uuid(v);
        EXPECT_EQ(static_cast<uint64_t>(uuid), v);
    }
}

// FromString handles edge cases
TEST(UUIDTest, FromStringEdgeCases) {
    // Zero UUID
    UUID zero = UUID::FromString("0000000000000000");
    EXPECT_EQ(static_cast<uint64_t>(zero), 0ULL);

    // Max UUID
    UUID maxVal = UUID::FromString("ffffffffffffffff");
    EXPECT_EQ(static_cast<uint64_t>(maxVal), 0xFFFFFFFFFFFFFFFFULL);
}

// Default UUID is non-zero (randomly generated)
TEST(UUIDTest, DefaultConstructorIsRandom) {
    UUID defaultUUID;
    EXPECT_NE(static_cast<uint64_t>(defaultUUID), 0ULL);
}

// UUID can be used as a key in unordered containers (via std::hash)
TEST(UUIDTest, HashableInUnorderedSet) {
    std::unordered_set<UUID> set;
    UUID a;
    UUID b;
    set.insert(a);
    set.insert(b);
    EXPECT_EQ(set.size(), 2);

    UUID aCopy = a;
    set.insert(aCopy);
    EXPECT_EQ(set.size(), 2); // Duplicate not inserted
}

} // namespace
} // namespace Prisma
