#include <gtest/gtest.h>
#include <string>

TEST(Sanity, TrueIsTrue) {
    EXPECT_TRUE(true);
}

TEST(Sanity, AdditionWorks) {
    EXPECT_EQ(1 + 1, 2);
}

TEST(Sanity, StringCompare) {
    std::string hello = "hello";
    EXPECT_EQ(hello, "hello");
}
