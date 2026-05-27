#pragma once

#include "Export.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Prisma {
namespace Animation {

/// 骨骼定义
struct ENGINE_API Bone {
    std::string name;               ///< 骨骼名称
    uint32_t index = UINT32_MAX;    ///< 骨骼在数组中的索引
    int32_t parentIndex = -1;       ///< 父骨骼索引（-1 表示根骨骼）
    glm::dmat4 inverseBindMatrix{1.0}; ///< 绑定姿势的逆矩阵（用于蒙皮）
    glm::dmat4 localTransform{1.0};    ///< 当前局部变换矩阵

    Bone() = default;

    Bone(const std::string& boneName, uint32_t idx, int32_t parentIdx)
        : name(boneName), index(idx), parentIndex(parentIdx) {}
};

/// 骨骼层级
class ENGINE_API Skeleton {
public:
    Skeleton() = default;
    ~Skeleton() = default;

    // 不可拷贝，可移动
    Skeleton(const Skeleton&) = default;
    Skeleton& operator=(const Skeleton&) = default;
    Skeleton(Skeleton&&) noexcept = default;
    Skeleton& operator=(Skeleton&&) noexcept = default;

    /// 添加骨骼，返回其索引
    uint32_t AddBone(const std::string& name, int32_t parentIndex = -1);

    /// 通过名称查找骨骼索引，未找到返回 UINT32_MAX
    int32_t GetBoneIndex(const std::string& name) const;

    /// 获取骨骼引用
    Bone& GetBone(uint32_t index);
    const Bone& GetBone(uint32_t index) const;

    /// 获取骨骼数量
    size_t GetBoneCount() const { return m_bones.size(); }

    /// 设置绑定姿势逆矩阵
    void SetInverseBindMatrix(uint32_t index, const glm::dmat4& invBind);

    /// 计算所有骨骼的最终变换矩阵（从根到叶，依次累积局部变换）
    /// 返回数组长度 = GetBoneCount()，顺序与骨骼索引一致
    std::vector<glm::dmat4> ComputeFinalBoneMatrices() const;

    /// 计算蒙皮矩阵（最终变换 * 绑定姿势逆矩阵）
    std::vector<glm::dmat4> ComputeSkinningMatrices() const;

    /// 获取所有骨骼
    const std::vector<Bone>& GetBones() const { return m_bones; }

    /// 重置所有骨骼局部变换为单位矩阵
    void ResetPose();

private:
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, uint32_t> m_nameToIndex;
};

} // namespace Animation
} // namespace Prisma
