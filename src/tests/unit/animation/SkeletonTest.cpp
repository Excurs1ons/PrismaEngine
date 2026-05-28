#include <gtest/gtest.h>
#include "animation/Skeleton.h"
#include <glm/gtc/type_ptr.hpp>

namespace Prisma {
namespace Animation {
namespace {

// ============================================================
// 骨骼添加与基本属性
// ============================================================

// 单个根骨骼
TEST(SkeletonTest, AddSingleRootBone) {
    Skeleton skeleton;
    uint32_t idx = skeleton.AddBone("root");
    EXPECT_EQ(idx, 0u);
    EXPECT_EQ(skeleton.GetBoneCount(), 1u);

    const Bone& bone = skeleton.GetBone(0);
    EXPECT_EQ(bone.name, "root");
    EXPECT_EQ(bone.index, 0u);
    EXPECT_EQ(bone.parentIndex, -1);
}

// 带父骨骼的层级
TEST(SkeletonTest, AddChildBone) {
    Skeleton skeleton;
    uint32_t rootIdx = skeleton.AddBone("root");
    uint32_t childIdx = skeleton.AddBone("child", static_cast<int32_t>(rootIdx));

    EXPECT_EQ(rootIdx, 0u);
    EXPECT_EQ(childIdx, 1u);
    EXPECT_EQ(skeleton.GetBoneCount(), 2u);

    const Bone& child = skeleton.GetBone(1);
    EXPECT_EQ(child.parentIndex, 0);
}

// 多个子骨骼共享同一父骨骼
TEST(SkeletonTest, MultipleChildrenSameParent) {
    Skeleton skeleton;
    skeleton.AddBone("root");
    uint32_t c1 = skeleton.AddBone("childA", 0);
    uint32_t c2 = skeleton.AddBone("childB", 0);
    uint32_t c3 = skeleton.AddBone("childC", 0);

    EXPECT_EQ(c1, 1u);
    EXPECT_EQ(c2, 2u);
    EXPECT_EQ(c3, 3u);

    EXPECT_EQ(skeleton.GetBone(1).parentIndex, 0);
    EXPECT_EQ(skeleton.GetBone(2).parentIndex, 0);
    EXPECT_EQ(skeleton.GetBone(3).parentIndex, 0);
}

// 深层嵌套层级
TEST(SkeletonTest, DeepHierarchy) {
    Skeleton skeleton;
    // root -> a -> b -> c
    uint32_t r = skeleton.AddBone("root");
    uint32_t a = skeleton.AddBone("a", static_cast<int32_t>(r));
    uint32_t b = skeleton.AddBone("b", static_cast<int32_t>(a));
    uint32_t c = skeleton.AddBone("c", static_cast<int32_t>(b));

    EXPECT_EQ(r, 0u);
    EXPECT_EQ(a, 1u);
    EXPECT_EQ(b, 2u);
    EXPECT_EQ(c, 3u);

    EXPECT_EQ(skeleton.GetBone(1).parentIndex, 0);
    EXPECT_EQ(skeleton.GetBone(2).parentIndex, 1);
    EXPECT_EQ(skeleton.GetBone(3).parentIndex, 2);
}

// ============================================================
// 骨骼名称查找
// ============================================================

TEST(SkeletonTest, GetBoneIndexFound) {
    Skeleton skeleton;
    skeleton.AddBone("root");
    skeleton.AddBone("hip");
    skeleton.AddBone("spine");

    EXPECT_EQ(skeleton.GetBoneIndex("root"), 0);
    EXPECT_EQ(skeleton.GetBoneIndex("hip"), 1);
    EXPECT_EQ(skeleton.GetBoneIndex("spine"), 2);
}

TEST(SkeletonTest, GetBoneIndexNotFound) {
    Skeleton skeleton;
    skeleton.AddBone("a");
    skeleton.AddBone("b");

    EXPECT_EQ(skeleton.GetBoneIndex("nonexistent"), -1);
}

TEST(SkeletonTest, GetBoneIndexEmptySkeleton) {
    Skeleton skeleton;
    EXPECT_EQ(skeleton.GetBoneIndex("anything"), -1);
}

// ============================================================
// GetBone 边界行为
// ============================================================

TEST(SkeletonTest, GetBoneValidRange) {
    Skeleton skeleton;
    skeleton.AddBone("bone0");
    skeleton.AddBone("bone1");

    EXPECT_NO_THROW(skeleton.GetBone(0));
    EXPECT_NO_THROW(skeleton.GetBone(1));

    const Bone& b0 = skeleton.GetBone(0);
    EXPECT_EQ(b0.name, "bone0");
}

TEST(SkeletonTest, GetBoneOutOfRangeThrows) {
    Skeleton skeleton;
    skeleton.AddBone("only");

    EXPECT_THROW(skeleton.GetBone(1), std::out_of_range);
    EXPECT_THROW(skeleton.GetBone(100), std::out_of_range);
    // UINT32_MAX is way out of range
    EXPECT_THROW(skeleton.GetBone(UINT32_MAX), std::out_of_range);
}

TEST(SkeletonTest, GetBoneConstOutOfRangeThrows) {
    const Skeleton skeleton; // empty
    EXPECT_THROW(skeleton.GetBone(0), std::out_of_range);
}

// ============================================================
// 绑定姿势逆矩阵
// ============================================================

TEST(SkeletonTest, SetInverseBindMatrix) {
    Skeleton skeleton;
    skeleton.AddBone("bone");
    glm::dmat4 invBind = glm::translate(glm::dmat4(1.0), glm::dvec3(1.0, 2.0, 3.0));
    skeleton.SetInverseBindMatrix(0, invBind);

    const Bone& bone = skeleton.GetBone(0);
    EXPECT_EQ(bone.inverseBindMatrix, invBind);
}

TEST(SkeletonTest, SetInverseBindMatrixOutOfRange) {
    Skeleton skeleton;
    skeleton.AddBone("bone");
    EXPECT_THROW(skeleton.SetInverseBindMatrix(1, glm::dmat4(1.0)), std::out_of_range);
}

TEST(SkeletonTest, DefaultInverseBindIsIdentity) {
    Skeleton skeleton;
    skeleton.AddBone("bone");
    const Bone& bone = skeleton.GetBone(0);
    EXPECT_EQ(bone.inverseBindMatrix, glm::dmat4(1.0));
}

// ============================================================
// 最终变换矩阵计算
// ============================================================

TEST(SkeletonTest, ComputeFinalMatricesSingleRoot) {
    Skeleton skeleton;
    skeleton.AddBone("root");
    // 根骨骼 localTransform 默认是单位矩阵
    auto matrices = skeleton.ComputeFinalBoneMatrices();
    ASSERT_EQ(matrices.size(), 1u);
    EXPECT_EQ(matrices[0], glm::dmat4(1.0));
}

TEST(SkeletonTest, ComputeFinalMatricesParentChild) {
    Skeleton skeleton;
    skeleton.AddBone("root");
    skeleton.AddBone("child", 0);

    // 给子骨骼一个局部平移
    glm::dmat4 childLocal = glm::translate(glm::dmat4(1.0), glm::dvec3(5.0, 0.0, 0.0));
    skeleton.GetBone(1).localTransform = childLocal;

    auto matrices = skeleton.ComputeFinalBoneMatrices();
    ASSERT_EQ(matrices.size(), 2u);

    // 根骨骼：单位矩阵
    EXPECT_EQ(matrices[0], glm::dmat4(1.0));
    // 子骨骼：父变换 * 局部变换 = I * childLocal = childLocal
    EXPECT_EQ(matrices[1], childLocal);
}

TEST(SkeletonTest, ComputeFinalMatricesMultiLevel) {
    Skeleton skeleton;
    // root -> mid -> tip
    skeleton.AddBone("root");
    skeleton.AddBone("mid", 0);
    skeleton.AddBone("tip", 1);

    // root 平移 (1,0,0)
    // mid 平移 (0,2,0)
    // tip 平移 (0,0,3)
    glm::dmat4 rootLocal = glm::translate(glm::dmat4(1.0), glm::dvec3(1.0, 0.0, 0.0));
    glm::dmat4 midLocal  = glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, 2.0, 0.0));
    glm::dmat4 tipLocal  = glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, 0.0, 3.0));

    skeleton.GetBone(0).localTransform = rootLocal;
    skeleton.GetBone(1).localTransform = midLocal;
    skeleton.GetBone(2).localTransform = tipLocal;

    auto matrices = skeleton.ComputeFinalBoneMatrices();
    ASSERT_EQ(matrices.size(), 3u);

    EXPECT_EQ(matrices[0], rootLocal);
    // mid 最终：rootLocal * midLocal
    glm::dmat4 expectedMid = rootLocal * midLocal;
    EXPECT_EQ(matrices[1], expectedMid);
    // tip 最终：rootLocal * midLocal * tipLocal
    glm::dmat4 expectedTip = rootLocal * midLocal * tipLocal;
    EXPECT_EQ(matrices[2], expectedTip);
}

TEST(SkeletonTest, ComputeFinalMatricesAfterModification) {
    Skeleton skeleton;
    skeleton.AddBone("root");
    skeleton.AddBone("child", 0);

    // 修改后重新计算
    skeleton.GetBone(0).localTransform =
        glm::translate(glm::dmat4(1.0), glm::dvec3(10.0, 0.0, 0.0));
    skeleton.GetBone(1).localTransform =
        glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, 20.0, 0.0));

    auto matrices1 = skeleton.ComputeFinalBoneMatrices();
    // 再次修改
    skeleton.GetBone(1).localTransform =
        glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, 99.0, 0.0));
    auto matrices2 = skeleton.ComputeFinalBoneMatrices();
    // 两次应该不同
    EXPECT_NE(matrices1[1], matrices2[1]);
}

// ============================================================
// 蒙皮矩阵计算
// ============================================================

TEST(SkeletonTest, ComputeSkinningMatrices) {
    Skeleton skeleton;
    skeleton.AddBone("root");

    glm::dmat4 invBind = glm::translate(glm::dmat4(1.0), glm::dvec3(-1.0, 0.0, 0.0));
    skeleton.SetInverseBindMatrix(0, invBind);
    skeleton.GetBone(0).localTransform =
        glm::translate(glm::dmat4(1.0), glm::dvec3(5.0, 0.0, 0.0));

    auto skinning = skeleton.ComputeSkinningMatrices();
    ASSERT_EQ(skinning.size(), 1u);

    // skin = final * invBind = (I * local) * invBind
    glm::dmat4 expected = skeleton.GetBone(0).localTransform * invBind;
    EXPECT_EQ(skinning[0], expected);
}

// ============================================================
// ResetPose
// ============================================================

TEST(SkeletonTest, ResetPoseSetsAllToIdentity) {
    Skeleton skeleton;
    skeleton.AddBone("root");
    skeleton.AddBone("child", 0);

    skeleton.GetBone(0).localTransform =
        glm::translate(glm::dmat4(1.0), glm::dvec3(1.0, 2.0, 3.0));
    skeleton.GetBone(1).localTransform =
        glm::translate(glm::dmat4(1.0), glm::dvec3(4.0, 5.0, 6.0));

    skeleton.ResetPose();

    EXPECT_EQ(skeleton.GetBone(0).localTransform, glm::dmat4(1.0));
    EXPECT_EQ(skeleton.GetBone(1).localTransform, glm::dmat4(1.0));
}

TEST(SkeletonTest, ResetPoseOnEmptySkeleton) {
    Skeleton skeleton;
    EXPECT_NO_THROW(skeleton.ResetPose());
}

// ============================================================
// GetBones（返回所有骨骼）
// ============================================================

TEST(SkeletonTest, GetBonesReturnsAll) {
    Skeleton skeleton;
    skeleton.AddBone("a");
    skeleton.AddBone("b");
    skeleton.AddBone("c");

    const auto& bones = skeleton.GetBones();
    ASSERT_EQ(bones.size(), 3u);
    EXPECT_EQ(bones[0].name, "a");
    EXPECT_EQ(bones[1].name, "b");
    EXPECT_EQ(bones[2].name, "c");
}

TEST(SkeletonTest, GetBonesEmpty) {
    const Skeleton skeleton;
    const auto& bones = skeleton.GetBones();
    EXPECT_TRUE(bones.empty());
}

// ============================================================
// GetBoneCount
// ============================================================

TEST(SkeletonTest, BoneCount) {
    Skeleton skeleton;
    EXPECT_EQ(skeleton.GetBoneCount(), 0u);
    skeleton.AddBone("a");
    EXPECT_EQ(skeleton.GetBoneCount(), 1u);
    skeleton.AddBone("b");
    EXPECT_EQ(skeleton.GetBoneCount(), 2u);
    skeleton.AddBone("c");
    EXPECT_EQ(skeleton.GetBoneCount(), 3u);
}

// ============================================================
// 移动语义
// ============================================================

TEST(SkeletonTest, MoveConstructor) {
    Skeleton original;
    original.AddBone("root");
    original.AddBone("child", 0);

    Skeleton moved(std::move(original));
    EXPECT_EQ(moved.GetBoneCount(), 2u);
    EXPECT_EQ(moved.GetBoneIndex("root"), 0);
    EXPECT_EQ(moved.GetBoneIndex("child"), 1);
}

TEST(SkeletonTest, MoveAssignment) {
    Skeleton original;
    original.AddBone("a");
    original.AddBone("b", 0);

    Skeleton assigned;
    assigned = std::move(original);
    EXPECT_EQ(assigned.GetBoneCount(), 2u);
    EXPECT_EQ(assigned.GetBoneIndex("a"), 0);
}

} // namespace
} // namespace Animation
} // namespace Prisma
