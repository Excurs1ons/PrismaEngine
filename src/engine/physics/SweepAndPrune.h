#pragma once

#include "CollisionSystem.h"
#include <vector>
#include <algorithm>
#include <cstdint>

namespace Prisma {
    namespace Physics {

        /**
         * @brief Sweep and Prune 宽阶段碰撞检测
         *
         * 模板参数 Axis: 0=X, 1=Y, 2=Z
         * O(n log n) 排序 + O(n) 扫描，替代 O(n²) 暴力检测
         *
         * 算法：
         *   1. 按 min[Axis] 排序所有 AABB
         *   2. 遍历排序后的 AABB，维护活动列表
         *   3. 从活动列表中移除 max[Axis] < current.min[Axis] 的条目
         *   4. 对活动列表中剩余的条目执行完整 3D AABB 重叠检测
         */
        template<int Axis = 0>
        class SweepAndPrune {
        public:
            SweepAndPrune() = default;

            /**
             * @brief 更新 AABB 列表并计算所有重叠对
             * @param aabbs 所有参与检测的 AABB（索引与外部数组一致）
             */
            void update(const std::vector<AABB>& aabbs) {
                m_overlappingPairs.clear();
                size_t count = aabbs.size();
                if (count < 2) return;

                // 构建带索引的条目并按 min[Axis] 排序
                m_entries.resize(count);
                for (size_t i = 0; i < count; ++i) {
                    m_entries[i] = { static_cast<uint32_t>(i), aabbs[i] };
                }

                std::sort(m_entries.begin(), m_entries.end(),
                    [](const Entry& a, const Entry& b) {
                        return getMin(a.aabb) < getMin(b.aabb);
                    });

                // 活动列表（存储 m_entries 中的索引）
                m_activeList.clear();
                m_activeList.reserve(count);

                for (size_t i = 0; i < count; ++i) {
                    const auto& current = m_entries[i];
                    double currentMin = getMin(current.aabb);

                    // 从活动列表中移除已结束的条目（其最大边界在当前最小边界左侧）
                    auto activeIt = m_activeList.begin();
                    while (activeIt != m_activeList.end()) {
                        const auto& active = m_entries[*activeIt];
                        if (getMax(active.aabb) < currentMin) {
                            activeIt = m_activeList.erase(activeIt);
                        } else {
                            ++activeIt;
                        }
                    }

                    // 检查当前条目与活动列表中所有条目在 3D 空间中的重叠
                    for (uint32_t activeIdx : m_activeList) {
                        const auto& active = m_entries[activeIdx];
                        if (current.aabb.intersects(active.aabb)) {
                            m_overlappingPairs.emplace_back(active.bodyIndex, current.bodyIndex);
                        }
                    }

                    m_activeList.push_back(static_cast<uint32_t>(i));
                }
            }

            const std::vector<std::pair<uint32_t, uint32_t>>& getOverlappingPairs() const {
                return m_overlappingPairs;
            }

            void clear() {
                m_entries.clear();
                m_activeList.clear();
                m_overlappingPairs.clear();
            }

        private:
            struct Entry {
                uint32_t bodyIndex;
                AABB aabb;
            };

            static double getMin(const AABB& aabb) {
                if constexpr (Axis == 0) return aabb.minX;
                if constexpr (Axis == 1) return aabb.minY;
                return aabb.minZ;
            }

            static double getMax(const AABB& aabb) {
                if constexpr (Axis == 0) return aabb.maxX;
                if constexpr (Axis == 1) return aabb.maxY;
                return aabb.maxZ;
            }

            std::vector<Entry> m_entries;                         // 排序后的条目
            std::vector<uint32_t> m_activeList;                   // 活动列表（m_entries 索引）
            std::vector<std::pair<uint32_t, uint32_t>> m_overlappingPairs; // 重叠对
        };

    } // namespace Physics
} // namespace Prisma
