#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <array>
#include <functional>
#include <cmath>

namespace PrismaEngine {
    namespace Graphic {

        /**
         * @brief 视锥体 - 用于剔除不可见物体
         *
         * 对应 OpenGL/图形学的视锥剔除技术
         * 由 6 个平面组成：上、下、左、右、近、远
         */
        class Frustum {
        public:
            // ========== 平面定义 ==========

            /**
             * @brief 平面 - 使用法线-距离形式表示
             *
             * 平面方程：normal · point + distance = 0
             */
            struct Plane {
                glm::dvec3 normal;    // 平面法线（单位向量）
                double distance;     // 到原点的距离（带符号）

                /**
                 * @brief 归一化平面（确保法线是单位向量）
                 */
                void normalize() {
                    double len = glm::length(normal);
                    if (len > 1e-9) {
                        normal /= len;
                        distance /= len;
                    }
                }

                /**
                 * @brief 计算点到平面的有符号距离
                 * @param point 测试点
                 * @return 正值表示在平面法线方向一侧，负值在另一侧
                 */
                double distanceToPoint(const glm::dvec3& point) const {
                    return glm::dot(normal, point) + distance;
                }
            };

            // ========== 构造函数 ==========

            Frustum() = default;

            /**
             * @brief 从视图-投影矩阵创建视锥
             * @param viewProjectionMatrix 视图-投影矩阵
             */
            void update(const glm::dmat4& viewProjectionMatrix) {
                // 提取视锥的 6 个平面
                // 左平面: col3 + col0
                m_planes[0] = extractPlane(viewProjectionMatrix,  1,  0); // Left
                // 右平面: col3 - col0
                m_planes[1] = extractPlane(viewProjectionMatrix, -1,  0); // Right
                // 下平面: col3 + col1
                m_planes[2] = extractPlane(viewProjectionMatrix,  1,  1); // Bottom
                // 上平面: col3 - col1
                m_planes[3] = extractPlane(viewProjectionMatrix, -1,  1); // Top
                // 近平面: col3 + col2
                m_planes[4] = extractPlane(viewProjectionMatrix,  1,  2); // Near
                // 远平面: col3 - col2
                m_planes[5] = extractPlane(viewProjectionMatrix, -1,  2); // Far
            }

            /**
             * @brief 从相机参数创建视锥
             * @param position 相机位置
             * @param forward 相机前方向量
             * @param up 相机上方向量
             * @param fovY 垂直视野角度（弧度）
             * @param aspect 宽高比
             * @param nearDistance 近裁剪面距离
             * @param farDistance 远裁剪面距离
             */
            void updateFromCamera(const glm::dvec3& position, const glm::dvec3& forward,
                                   const glm::dvec3& up, double fovY, double aspect,
                                   double nearDistance, double farDistance) {
                glm::dvec3 right = glm::normalize(glm::cross(forward, up));
                glm::dvec3 actualUp = glm::cross(right, forward);

                // 计算视锥参数
                double halfHeight = std::tan(fovY * 0.5);
                double halfWidth = halfHeight * aspect;

                glm::dvec3 nearCenter = position + forward * nearDistance;
                glm::dvec3 farCenter = position + forward * farDistance;

                glm::dvec3 nearHalfHeight = actualUp * (halfHeight * nearDistance);
                glm::dvec3 nearHalfWidth = right * (halfWidth * nearDistance);
                glm::dvec3 farHalfHeight = actualUp * (halfHeight * farDistance);
                glm::dvec3 farHalfWidth = right * (halfWidth * farDistance);

                // 计算视锥的 8 个角点
                glm::dvec3 ntl = nearCenter + nearHalfHeight - nearHalfWidth;  // Near-Top-Left
                glm::dvec3 ntr = nearCenter + nearHalfHeight + nearHalfWidth;  // Near-Top-Right
                glm::dvec3 nbl = nearCenter - nearHalfHeight - nearHalfWidth;  // Near-Bottom-Left
                glm::dvec3 nbr = nearCenter - nearHalfHeight + nearHalfWidth;  // Near-Bottom-Right
                glm::dvec3 ftl = farCenter + farHalfHeight - farHalfWidth;    // Far-Top-Left
                glm::dvec3 ftr = farCenter + farHalfHeight + farHalfWidth;    // Far-Top-Right
                glm::dvec3 fbl = farCenter - farHalfHeight - farHalfWidth;    // Far-Bottom-Left
                glm::dvec3 fbr = farCenter - farHalfHeight + farHalfWidth;    // Far-Bottom-Right

                // 构建视锥的 6 个平面（外法线）
                m_planes[0] = Plane::fromPoints(nbl, ftl, ntl);  // Left
                m_planes[1] = Plane::fromPoints(ntr, fbr, nbr);  // Right
                m_planes[2] = Plane::fromPoints(nbl, nbr, fbl);  // Bottom
                m_planes[3] = Plane::fromPoints(ntl, ntr, ftr);  // Top
                m_planes[4] = Plane::fromPoints(ntl, nbl, nbr);  // Near
                m_planes[5] = Plane::fromPoints(ftr, ftl, fbl);  // Far
            }

            // ========== 可见性检测 ==========

            /**
             * @brief 检测点是否在视锥内
             */
            bool isVisible(const glm::dvec3& point) const {
                for (const auto& plane : m_planes) {
                    if (plane.distanceToPoint(point) < 0) {
                        return false;
                    }
                }
                return true;
            }

            /**
             * @brief 检测球体是否在视锥内
             * @param center 球心
             * @param radius 半径
             */
            bool isVisible(const glm::dvec3& center, double radius) const {
                for (const auto& plane : m_planes) {
                    // 如果球心到平面的距离小于负的半径，球体完全在平面外侧
                    if (plane.distanceToPoint(center) < -radius) {
                        return false;
                    }
                }
                return true;
            }

            /**
             * @brief 检测 AABB 是否在视锥内
             * @param minAABB 包围盒最小点
             * @param maxAABB 包围盒最大点
             */
            bool isVisible(const glm::dvec3& minAABB, const glm::dvec3& maxAABB) const {
                // 测试 AABB 的 8 个角点
                // 优化：只测试最远的正顶点
                for (const auto& plane : m_planes) {
                    // 找到 AABB 在平面法线方向最远的顶点
                    glm::dvec3 positiveVertex(
                        plane.normal.x > 0 ? maxAABB.x : minAABB.x,
                        plane.normal.y > 0 ? maxAABB.y : minAABB.y,
                        plane.normal.z > 0 ? maxAABB.z : minAABB.z
                    );

                    // 如果最远的顶点都在平面外侧，则 AABB 不可见
                    if (plane.distanceToPoint(positiveVertex) < 0) {
                        return false;
                    }
                }
                return true;
            }

            /**
             * @brief 检测 AABB 是否在视锥内（重载版本）
             */
            bool isVisible(double minX, double minY, double minZ,
                           double maxX, double maxY, double maxZ) const {
                return isVisible(glm::dvec3(minX, minY, minZ), glm::dvec3(maxX, maxY, maxZ));
            }

            /**
             * @brief 批量检测 AABB 可见性
             * @param aabbs AABB 列表
             * @return 可见索引列表
             */
            std::vector<size_t> filterVisible(const std::vector<std::pair<glm::dvec3, glm::dvec3>>& aabbs) const {
                std::vector<size_t> visible;
                for (size_t i = 0; i < aabbs.size(); ++i) {
                    if (isVisible(aabbs[i].first, aabbs[i].second)) {
                        visible.push_back(i);
                    }
                }
                return visible;
            }

            // ========== 平面访问 ==========

            enum PlaneIndex {
                LEFT = 0,
                RIGHT = 1,
                BOTTOM = 2,
                TOP = 3,
                NEAR = 4,
                FAR = 5,
                COUNT = 6
            };

            const Plane& getPlane(PlaneIndex index) const {
                return m_planes[static_cast<size_t>(index)];
            }

        private:
            std::array<Plane, 6> m_planes;

            /**
             * @brief 从矩阵提取平面
             * @param matrix 视图-投影矩阵
             * @param sign 符号（用于选择矩阵的列）
             * @param row 行索引
             */
            static Plane extractPlane(const glm::dmat4& matrix, int sign, int row) {
                glm::dvec3 normal;
                normal.x = matrix[0][3] + matrix[0][row] * sign;
                normal.y = matrix[1][3] + matrix[1][row] * sign;
                normal.z = matrix[2][3] + matrix[2][row] * sign;

                double distance = matrix[3][3] + matrix[3][row] * sign;

                Plane plane;
                plane.normal = glm::normalize(normal);
                plane.distance = distance;

                return plane;
            }
        };

        /**
         * @brief 视锥剔除系统
         *
         * 管理视锥剔除，批量处理可见性检测
         */
        class FrustumCullingSystem {
        public:
            /**
             * @brief 剔除统计信息
             */
            struct CullingStats {
                size_t totalObjects = 0;
                size_t visibleObjects = 0;
                size_t culledObjects = 0;
                double cullingRatio = 0.0;  // 剔除比例 (0-1)

                void calculate() {
                    if (totalObjects > 0) {
                        cullingRatio = static_cast<double>(culledObjects) / totalObjects;
                    }
                }
            };

            FrustumCullingSystem() = default;

            /**
             * @brief 更新视锥
             */
            void updateFrustum(const glm::dmat4& viewProjectionMatrix) {
                m_frustum.update(viewProjectionMatrix);
            }

            void updateFrustum(const glm::dvec3& position, const glm::dvec3& forward,
                               const glm::dvec3& up, double fovY, double aspect,
                               double nearDistance, double farDistance) {
                m_frustum.updateFromCamera(position, forward, up, fovY, aspect, nearDistance, farDistance);
            }

            /**
             * @brief 获取当前视锥
             */
            const Frustum& getFrustum() const {
                return m_frustum;
            }

            /**
             * @brief 剔除不可见的 AABB
             * @return 可见对象的索引
             */
            template<typename AABBGetter>
            std::vector<size_t> cull(const AABBGetter& getter, size_t count) const {
                std::vector<size_t> visible;
                for (size_t i = 0; i < count; ++i) {
                    auto [minAABB, maxAABB] = getter(i);
                    if (m_frustum.isVisible(minAABB, maxAABB)) {
                        visible.push_back(i);
                    }
                }
                return visible;
            }

            /**
             * @brief 批量剔除（带统计）
             */
            template<typename AABBGetter>
            std::vector<size_t> cullWithStats(const AABBGetter& getter, size_t count, CullingStats& stats) const {
                stats.totalObjects = count;
                std::vector<size_t> visible;

                for (size_t i = 0; i < count; ++i) {
                    auto [minAABB, maxAABB] = getter(i);
                    if (m_frustum.isVisible(minAABB, maxAABB)) {
                        visible.push_back(i);
                    }
                }

                stats.visibleObjects = visible.size();
                stats.culledObjects = count - visible.size();
                stats.calculate();

                return visible;
            }

        private:
            Frustum m_frustum;
        };

    } // namespace Graphic
} // namespace PrismaEngine
