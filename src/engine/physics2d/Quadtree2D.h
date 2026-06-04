#pragma once

#include "AABB2D.h"
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace Prisma::Physics2D {

/**
 * @brief 2D Quadtree for spatial partitioning
 *
 * 用于加速 2D 物理世界的空间查询（碰撞检测、视野裁剪等）。
 * 非线程安全 —— 调用方需自行同步。
 *
 * 行为：
 * - 节点超过 m_maxElements 且深度 < m_maxDepth 时，分裂为 4 个子节点并重新分配元素
 * - 横跨多个象限的元素留在父节点（FitsInChild 检查）
 * - Query() 返回所有与查询范围重叠的 entityId
 * - Update() 实现为 Remove + Insert
 */
class Quadtree2D {
public:
    /**
     * @brief 构造四叉树
     * @param bounds     世界边界（覆盖所有可能的实体位置）
     * @param maxDepth   最大分裂深度（默认 8）
     * @param maxElements 每个节点最大元素数（触发分裂的阈值）
     */
    Quadtree2D(const AABB2D& bounds, uint32_t maxDepth = 8, uint32_t maxElements = 16);
    ~Quadtree2D() = default;

    /// @brief 插入实体
    void Insert(uint32_t entityId, const AABB2D& aabb);

    /// @brief 移除实体
    void Remove(uint32_t entityId);

    /// @brief 更新实体位置（Remove + Insert）
    void Update(uint32_t entityId, const AABB2D& newAABB);

    /**
     * @brief 查询与 range 重叠的所有实体
     * @param range 查询范围
     * @return 重叠的 entityId 列表
     */
    std::vector<uint32_t> Query(const AABB2D& range) const;

    /// @brief 清空所有节点和元素
    void Clear();

    /// @brief 获取当前节点总数
    uint32_t GetNodeCount() const { return static_cast<uint32_t>(m_nodes.size()); }

    /// @brief 获取所有元素总数
    uint32_t GetTotalElementCount() const;

private:
    struct Node {
        AABB2D bounds;
        std::vector<uint32_t> elements;      // 存储在该节点的 entityId
        uint32_t children[4] = {0, 0, 0, 0}; // 子节点索引（m_nodes 中的下标）
        bool isLeaf = true;                  // true 表示没有子节点
    };

    /// @brief 创建一个新节点并返回其索引
    uint32_t CreateNode(const AABB2D& bounds);

    /// @brief 分裂节点为 4 个子节点
    void SplitNode(uint32_t nodeIndex);

    /// @brief 内部插入递归
    void InsertInternal(uint32_t nodeIndex, uint32_t entityId, const AABB2D& aabb, uint32_t depth);

    /// @brief 内部查询递归
    void QueryInternal(uint32_t nodeIndex, const AABB2D& range, std::vector<uint32_t>& result) const;

    /// @brief 判断元素属于哪个象限（0-3，-1 表示跨象限）
    int GetQuadrant(const AABB2D& nodeBounds, const AABB2D& elementAABB) const;

    /// @brief 判断元素是否能完全放入某个子节点
    bool FitsInChild(const AABB2D& nodeBounds, const AABB2D& elementAABB) const;

    /// @brief 递归统计某节点的元素总数
    uint32_t GetElementCountInternal(uint32_t nodeIndex) const;

    /// @brief 从节点及其子节点中递归移除实体（仅内部使用）
    bool RemoveEntityFromNode(uint32_t nodeIndex, uint32_t entityId);

    AABB2D m_bounds;
    uint32_t m_maxDepth;
    uint32_t m_maxElements;
    std::vector<Node> m_nodes;
    std::unordered_map<uint32_t, AABB2D> m_entityMap; // entityId -> last known AABB
};

} // namespace Prisma::Physics2D
