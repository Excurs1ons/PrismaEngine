#pragma once

#include "Export.h"
#include "NavMesh.h"
#include "PathFinder.h"
#include "PathSmoothing.h"
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>
#include <limits>

namespace Prisma {
namespace Navigation {

/**
 * @brief 导航智能体状态
 */
enum class NavAgentState : uint8_t {
    Idle,           // 空闲
    Pathfinding,    // 计算路径中
    FollowingPath,  // 沿路径移动
    Arrived,        // 已到达目标
    Stuck           // 卡住
};

/**
 * @brief 导航智能体参数
 */
struct ENGINE_API NavAgentParams {
    double radius          = 0.3;    // 智能体半径
    double height          = 1.8;    // 智能体高度
    double speed           = 4.0;    // 移动速度 (m/s)
    double maxAcceleration = 8.0;    // 最大加速度 (m/s²)
    double stopDistance    = 0.2;    // 停止距离
    double arrivalRadius   = 0.5;    // 到达判定半径
    double turnSpeed       = 10.0;   // 转向速度 (rad/s)
    double obstacleRadius  = 2.0;    // 动态障碍物检测半径
    double maxSlope        = 45.0;   // 最大可走坡度
    float  stuckTimeout    = 2.0f;   // 卡住超时时间（秒）
};

/**
 * @brief 导航智能体 (NavAgent)
 *
 * 智能体在 NavMesh 上进行路径跟随，使用转向行为实现流畅移动。
 * 支持基本的动态障碍物避让（速度障碍法）。
 */
class ENGINE_API NavAgent {
public:
    NavAgent()
        : m_params(NavAgentParams())
        , m_position(0)
        , m_velocity(0)
        , m_state(NavAgentState::Idle)
        , m_targetWaypointIndex(0)
        , m_stuckTimer(0.0f)
    {}

    explicit NavAgent(const NavAgentParams& params)
        : m_params(params)
        , m_position(0)
        , m_velocity(0)
        , m_state(NavAgentState::Idle)
        , m_targetWaypointIndex(0)
        , m_stuckTimer(0.0f)
    {}

    virtual ~NavAgent() = default;

    // ========== 公共接口 ==========

    /** 设置目标位置并开始寻路 */
    void MoveTo(const glm::dvec3& target,
                const NavMesh* navMesh,
                const PathFinder* pathFinder = nullptr,
                const PathSmoother* smoother = nullptr) {
        m_target = target;
        m_state = NavAgentState::Pathfinding;

        // 如果提供了寻路器，立即计算路径
        if (pathFinder && navMesh) {
            ComputePath(target, navMesh, pathFinder, smoother);
        }
    }

    /** 停止移动 */
    void Stop() {
        m_velocity = glm::dvec3(0);
        m_waypoints.clear();
        m_polygonPath.clear();
        m_targetWaypointIndex = 0;
        m_state = NavAgentState::Idle;
        m_stuckTimer = 0.0f;
    }

    /** 暂停移动 */
    void Pause() {
        if (m_state == NavAgentState::FollowingPath) {
            m_state = NavAgentState::Idle;
        }
    }

    /** 恢复移动 */
    void Resume() {
        if (!m_waypoints.empty() && m_state == NavAgentState::Idle) {
            m_state = NavAgentState::FollowingPath;
        }
    }

    /**
     * @brief 每帧更新智能体
     * @param dt 时间步长
     * @param navMesh 导航网格（用于位置修正）
     * @param otherAgents 其他智能体列表（用于避让）
     */
    void Update(double dt,
                const NavMesh* navMesh = nullptr,
                std::vector<NavAgent*>* otherAgents = nullptr) {
        if (m_state != NavAgentState::FollowingPath) {
            return;
        }

        if (m_waypoints.empty()) {
            m_state = NavAgentState::Arrived;
            return;
        }

        // 检查当前目标航点
        if (m_targetWaypointIndex >= m_waypoints.size()) {
            m_state = NavAgentState::Arrived;
            m_velocity = glm::dvec3(0);
            return;
        }

        glm::dvec3 targetWaypoint = m_waypoints[m_targetWaypointIndex];
        glm::dvec3 toTarget = targetWaypoint - m_position;
        double distToTarget = glm::length(toTarget);

        // 到达当前航点
        if (distToTarget < m_params.arrivalRadius) {
            ++m_targetWaypointIndex;

            if (m_targetWaypointIndex >= m_waypoints.size()) {
                // 所有航点已走完
                m_state = NavAgentState::Arrived;
                m_velocity = glm::dvec3(0);
                return;
            }

            targetWaypoint = m_waypoints[m_targetWaypointIndex];
            toTarget = targetWaypoint - m_position;
            distToTarget = glm::length(toTarget);
        }

        // --- 转向行为：向目标移动 ---
        glm::dvec3 desiredVelocity = (distToTarget > 1e-6)
            ? (toTarget / distToTarget) * m_params.speed
            : glm::dvec3(0);

        // 到达终点时减速
        bool isLastWaypoint = (m_targetWaypointIndex == m_waypoints.size() - 1);
        if (isLastWaypoint && distToTarget < m_params.stopDistance * 3.0) {
            double slowdownFactor = distToTarget / (m_params.stopDistance * 3.0);
            desiredVelocity *= std::clamp(slowdownFactor, 0.0, 1.0);
        }

        // --- 动态障碍物避让（速度障碍法） ---
        if (otherAgents && !otherAgents->empty()) {
            desiredVelocity = ApplyVelocityObstacleAvoidance(
                desiredVelocity, dt, *otherAgents);
        }

        // --- 加速度限制 ---
        glm::dvec3 acceleration = desiredVelocity - m_velocity;
        double accelLen = glm::length(acceleration);
        if (accelLen > m_params.maxAcceleration * dt) {
            acceleration = (acceleration / accelLen) * m_params.maxAcceleration * dt;
        }

        m_velocity += acceleration;

        // --- 速度限制 ---
        double speed = glm::length(m_velocity);
        if (speed > m_params.speed) {
            m_velocity = (m_velocity / speed) * m_params.speed;
        }

        // --- 位置更新 ---
        m_position += m_velocity * dt;

        // --- 投影到导航网格 ---
        if (navMesh) {
            SnapToNavMesh(navMesh);
        }

        // --- 卡住检测 ---
        if (speed < 0.01) {
            m_stuckTimer += static_cast<float>(dt);
            if (m_stuckTimer > m_params.stuckTimeout) {
                m_state = NavAgentState::Stuck;
                LOG_WARNING("NavAgent", "Agent stuck at ({:.2f}, {:.2f}, {:.2f})",
                            m_position.x, m_position.y, m_position.z);
            }
        } else {
            m_stuckTimer = 0.0f;
        }

        // --- 检查是否到达最终目标 ---
        if (isLastWaypoint && distToTarget < m_params.stopDistance) {
            m_state = NavAgentState::Arrived;
            m_velocity = glm::dvec3(0);
        }
    }

    /** 设置路径（外部计算后传入） */
    void SetPath(const std::vector<glm::dvec3>& waypoints) {
        m_waypoints = waypoints;
        m_targetWaypointIndex = 0;
        if (!m_waypoints.empty()) {
            m_state = NavAgentState::FollowingPath;
        }
    }

    void SetPath(const PathResult& pathResult) {
        m_waypoints = pathResult.waypoints;
        m_polygonPath = pathResult.polygonPath;
        m_targetWaypointIndex = 0;
        if (!m_waypoints.empty()) {
            m_state = NavAgentState::FollowingPath;
        }
    }

    // ========== 访问器 ==========

    NavAgentState GetState() const { return m_state; }
    const glm::dvec3& GetPosition() const { return m_position; }
    const glm::dvec3& GetVelocity() const { return m_velocity; }
    const glm::dvec3& GetTarget() const { return m_target; }
    const NavAgentParams& GetParams() const { return m_params; }
    NavAgentParams& GetParams() { return m_params; }

    void SetPosition(const glm::dvec3& pos) { m_position = pos; }
    void SetVelocity(const glm::dvec3& vel) { m_velocity = vel; }

    size_t GetWaypointCount() const { return m_waypoints.size(); }
    size_t GetCurrentWaypointIndex() const { return m_targetWaypointIndex; }
    const std::vector<glm::dvec3>& GetWaypoints() const { return m_waypoints; }

    bool IsMoving() const {
        return m_state == NavAgentState::FollowingPath
            || m_state == NavAgentState::Pathfinding;
    }

    bool HasArrived() const { return m_state == NavAgentState::Arrived; }
    bool IsStuck() const { return m_state == NavAgentState::Stuck; }

    /** 获取当前朝向（面向速度方向） */
    glm::dvec3 GetForward() const {
        double speed = glm::length(m_velocity);
        if (speed > 0.01) {
            return m_velocity / speed;
        }
        return glm::dvec3(0, 0, 1);
    }

private:
    /**
     * @brief 计算路径（内部调用 PathFinder）
     */
    void ComputePath(const glm::dvec3& target,
                     const NavMesh* navMesh,
                     const PathFinder* pathFinder,
                     const PathSmoother* smoother) {
        if (!navMesh || !pathFinder) {
            m_state = NavAgentState::Idle;
            return;
        }

        PathResult result = pathFinder->FindPath(m_position, target, *navMesh);

        if (result.found) {
            if (smoother) {
                result = smoother->Smooth(result, *navMesh);
            }
            m_waypoints = result.waypoints;
            m_polygonPath = result.polygonPath;
            m_targetWaypointIndex = 0;
            m_state = NavAgentState::FollowingPath;
        } else {
            LOG_WARNING("NavAgent", "Path not found from ({:.2f}, {:.2f}, {:.2f}) "
                        "to ({:.2f}, {:.2f}, {:.2f})",
                        m_position.x,                         m_position.y, m_position.z,
                        target.x, target.y, target.z);
            m_state = NavAgentState::Idle;
        }
    }

    /**
     * @brief 将智能体位置吸附到导航网格表面
     */
    void SnapToNavMesh(const NavMesh* navMesh) {
        int32_t polyIdx = navMesh->FindPolygonContainingPoint(m_position);
        if (polyIdx < 0) {
            // 不在网格上，尝试修正
            glm::dvec3 closest;
            if (navMesh->FindClosestPolygon(m_position, polyIdx, closest)) {
                m_position = closest;
            }
        } else {
            // 投影到多边形平面
            const auto& poly = navMesh->GetPolygon(polyIdx);
            glm::dvec3 normal = poly.GetNormal();
            if (glm::length(normal) > 1e-10) {
                glm::dvec3 toVertex = m_position - poly.vertices[0];
                double dist = glm::dot(toVertex, normal);
                m_position -= normal * dist;
            }
        }
    }

    /**
     * @brief 速度障碍法动态避让
     *
     * 对每个附近的智能体，计算碰撞时间，
     * 然后在速度空间中避开碰撞区域。
     */
    glm::dvec3 ApplyVelocityObstacleAvoidance(
        const glm::dvec3& desiredVelocity,
        double dt,
        std::vector<NavAgent*>& otherAgents) const {

        glm::dvec3 adjustedVel = desiredVelocity;

        for (auto* other : otherAgents) {
            if (!other || other == this) continue;

            glm::dvec3 relativePos = other->m_position - m_position;
            glm::dvec3 relativeVel = m_velocity - other->m_velocity;

            double distSq = glm::length2(relativePos);
            double combinedRadius = m_params.radius + other->m_params.radius;
            double combinedRadiusSq = combinedRadius * combinedRadius;

            // 跳过太远的智能体
            double obstacleRadiusSq = m_params.obstacleRadius * m_params.obstacleRadius;
            if (distSq > obstacleRadiusSq) continue;

            // 如果相对速度很小，跳过
            double relSpeedSq = glm::length2(relativeVel);
            if (relSpeedSq < 1e-6) {
                // 静态情况：如果太近，直接推开
                if (distSq < combinedRadiusSq && distSq > 1e-6) {
                    double dist = std::sqrt(distSq);
                    glm::dvec3 pushDir = relativePos / dist;
                    double pushStrength = (combinedRadius - dist) / combinedRadius;
                    adjustedVel -= pushDir * pushStrength * m_params.speed * 0.5;
                }
                continue;
            }

            // 计算最近接近时间
            double relSpeed = std::sqrt(relSpeedSq);
            glm::dvec3 relVelNorm = relativeVel / relSpeed;
            double closestApproach = glm::dot(relativePos, relVelNorm);

            // 如果正在远离，跳过
            if (closestApproach < 0) continue;

            // 最近距离
            glm::dvec3 closestPoint = relativePos - relVelNorm * closestApproach;
            double closestDistSq = glm::length2(closestPoint);

            if (closestDistSq < combinedRadiusSq) {
                // 会碰撞！计算避让向量
                double avoidanceStrength = 1.0 - closestDistSq / combinedRadiusSq;

                glm::dvec3 avoidanceDir;
                if (closestDistSq > 1e-10) {
                    avoidanceDir = glm::normalize(closestPoint);
                } else {
                    // 正对方向，随机侧向避让
                    avoidanceDir = glm::normalize(
                        glm::cross(relativePos, glm::dvec3(0, 1, 0)));
                    if (glm::length(avoidanceDir) < 1e-10) {
                        avoidanceDir = glm::dvec3(1, 0, 0);
                    }
                }

                // 向侧面避开
                adjustedVel += avoidanceDir * m_params.speed * avoidanceStrength * 0.3;
            }
        }

        // 重新限制速度大小
        double speed = glm::length(adjustedVel);
        if (speed > m_params.speed) {
            adjustedVel = (adjustedVel / speed) * m_params.speed;
        }

        return adjustedVel;
    }

    NavAgentParams       m_params;
    glm::dvec3           m_position;
    glm::dvec3           m_velocity;
    glm::dvec3           m_target;

    NavAgentState        m_state;
    std::vector<glm::dvec3> m_waypoints;
    std::vector<int32_t>    m_polygonPath;
    size_t               m_targetWaypointIndex;

    float                m_stuckTimer;
};

} // namespace Navigation
} // namespace Prisma
