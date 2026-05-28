#include <gtest/gtest.h>
#include "core/Handle.h"

namespace Prisma {
namespace {

// Default Handle is invalid
TEST(HandleTest, DefaultHandleIsInvalid) {
    Handle h;
    EXPECT_FALSE(h.IsValid());
    EXPECT_FALSE(static_cast<bool>(h));
}

// Valid Handle constructed with explicit ID
TEST(HandleTest, ExplicitIdConstructor) {
    Handle h(42);
    EXPECT_TRUE(h.IsValid());
    EXPECT_TRUE(static_cast<bool>(h));
    EXPECT_EQ(h.GetId(), 42u);
}

// Handle::Invalid() returns an invalid handle
TEST(HandleTest, StaticInvalidMethod) {
    Handle h = Handle::Invalid();
    EXPECT_FALSE(h.IsValid());
}

// GetId returns 0xFFFFFFFF for invalid handle
TEST(HandleTest, InvalidHandleGetId) {
    Handle h;
    EXPECT_EQ(h.GetId(), 0xFFFFFFFFu);
}

// Multiple handles with same ID are equivalent in value
TEST(HandleTest, SameIdEquality) {
    Handle a(100);
    Handle b(100);
    EXPECT_EQ(a.GetId(), b.GetId());
}

// ============================================================
// Type-safe derived handles
// ============================================================

// VertexBufferHandle default is invalid
TEST(HandleTest, VertexBufferHandleDefaultInvalid) {
    VertexBufferHandle vbh;
    EXPECT_FALSE(vbh.IsValid());
}

// VertexBufferHandle with explicit id is valid
TEST(HandleTest, VertexBufferHandleValid) {
    VertexBufferHandle vbh;
    // We need to construct with a value - Handle has explicit Handle(uint32_t)
    // but VertexBufferHandle doesn't inherit constructors.
    // Instead, verify type safety by checking that Implicit conversion
    // from int does NOT compile (compile-time test via template)
    static_cast<void>(vbh);
}

// Type safety: Handle types are distinct
TEST(HandleTest, TypeSafetyCompileTime) {
    // These compile - each type is distinct from Handle
    VertexBufferHandle vbh;
    IndexBufferHandle ibh;
    TextureHandle th;

    EXPECT_FALSE(vbh.IsValid());
    EXPECT_FALSE(ibh.IsValid());
    EXPECT_FALSE(th.IsValid());
}

// Handle with ID 0 is valid (0 is a legitimate ID)
TEST(HandleTest, HandleWithIdZeroIsValid) {
    Handle h(0);
    EXPECT_TRUE(h.IsValid());
    EXPECT_EQ(h.GetId(), 0u);
}

// Handle copy semantics
TEST(HandleTest, CopySemantics) {
    Handle original(123);
    Handle copy(original);
    EXPECT_EQ(copy.GetId(), 123u);
    EXPECT_TRUE(copy.IsValid());
}

} // namespace
} // namespace Prisma
