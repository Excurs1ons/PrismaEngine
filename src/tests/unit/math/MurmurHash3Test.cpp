#include <gtest/gtest.h>
#include "math/MurmurHash3.h"

using namespace Prisma::Math;

TEST(MurmurHash3, Deterministic32) {
    const char* data = "Hello, World!";
    Hash128 h1 = MurmurHash3::Hash(data, 13);
    Hash128 h2 = MurmurHash3::Hash(data, 13);
    EXPECT_EQ(h1.low, h2.low);
}

TEST(MurmurHash3, Deterministic128) {
    const char* data = "The quick brown fox jumps over the lazy dog";
    Hash128 h1 = MurmurHash3::Hash(data, 43);
    Hash128 h2 = MurmurHash3::Hash(data, 43);
    EXPECT_EQ(h1, h2);
}

TEST(MurmurHash3, EmptyInput) {
    Hash128 h = MurmurHash3::Hash("", 0);
    Hash128 h2 = MurmurHash3::Hash("", 0);
    EXPECT_EQ(h, h2);

    Hash128 hs = MurmurHash3::HashString("");
    EXPECT_EQ(h, hs);
}

TEST(MurmurHash3, SingleByte) {
    Hash128 h_a   = MurmurHash3::Hash("a", 1);
    Hash128 h_b   = MurmurHash3::Hash("b", 1);
    Hash128 h_a2  = MurmurHash3::Hash("a", 1);
    EXPECT_EQ(h_a, h_a2);
    EXPECT_NE(h_a, h_b);
    EXPECT_EQ(h_a, MurmurHash3::HashString("a"));
}

TEST(MurmurHash3, LargeData) {
    std::vector<uint8_t> largeData(4096);
    for (size_t i = 0; i < largeData.size(); ++i)
        largeData[i] = static_cast<uint8_t>(i & 0xFF);

    Hash128 h1 = MurmurHash3::Hash(largeData.data(), static_cast<int>(largeData.size()));
    Hash128 h2 = MurmurHash3::Hash(largeData.data(), static_cast<int>(largeData.size()));
    EXPECT_EQ(h1, h2);

    Hash128 h3 = MurmurHash3::Hash(largeData.data(), static_cast<int>(largeData.size()), 12345);
    EXPECT_NE(h1, h3);

    const uint8_t oddData[] = "ABCDEFGHIJKLMNOQQ";
    Hash128 h_odd  = MurmurHash3::Hash(oddData, 17);
    Hash128 h_odd2 = MurmurHash3::Hash(oddData, 17);
    EXPECT_EQ(h_odd, h_odd2);
}

TEST(MurmurHash3, Hash128ToString) {
    Hash128 h{0xABCDEF0123456789ULL, 0xDEADBEEFCAFEBABEULL};
    std::string s = h.ToString();
    EXPECT_EQ(s.length(), 32);
    EXPECT_EQ(s.substr(0, 16),  "abcdef0123456789");
    EXPECT_EQ(s.substr(16, 16), "deadbeefcafebabe");
}

TEST(MurmurHash3, Hash128FromString) {
    std::string hex = "abcdef0123456789deadbeefcafebabe";
    Hash128 h = Hash128::FromString(hex);
    EXPECT_EQ(h.low,  0xABCDEF0123456789ULL);
    EXPECT_EQ(h.high, 0xDEADBEEFCAFEBABEULL);

    Hash128 invalid = Hash128::FromString("short");
    EXPECT_EQ(invalid.low,  0);
    EXPECT_EQ(invalid.high, 0);
}
