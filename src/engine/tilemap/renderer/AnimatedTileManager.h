#pragma once

#include "../core/Tile.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace PrismaEngine {

// ============================================================================
// 动画瓦片变化
// ============================================================================

struct TileChange {
    int x, y;
    uint32_t newGid;
};

// ============================================================================
// 动画瓦片管理器
// ============================================================================

class AnimatedTileManager {
public:
    // 动画瓦片状态
    struct AnimatedTileState {
        int x, y;                    // 瓦片位置
        uint32_t baseGid;            // 基础 GID (图块集 firstGid)
        const Tile* tile;            // 瓦片定义指针
        size_t currentFrame;         // 当前帧索引
        float frameTimer;            // 帧计时器 (毫秒)

        AnimatedTileState()
            : x(0), y(0), baseGid(0), tile(nullptr), currentFrame(0), frameTimer(0.0f) {}
    };

    AnimatedTileManager() = default;
    ~AnimatedTileManager() = default;

    // 注册动画瓦片
    void RegisterTile(int x, int y, uint32_t gid, const Tile* tile);

    // 批量注册动画瓦片
    void RegisterTiles(const std::vector<AnimatedTileState>& tiles);

    // 取消注册指定位置的瓦片
    void UnregisterTile(int x, int y);

    // 清空所有动画瓦片
    void Clear();

    // 更新动画
    void Update(float deltaTime);

    // 获取变化的瓦片
    const std::vector<TileChange>& GetChangedTiles() const { return m_changedTiles; }

    // 清空变化列表
    void ClearChangedTiles() { m_changedTiles.clear(); }

    // 获取动画瓦片数量
    size_t GetAnimatedTileCount() const { return m_animatedTiles.size(); }

    // 检查指定位置是否有动画瓦片
    bool HasAnimatedTileAt(int x, int y) const;

    // 暂停/恢复动画
    void SetPaused(bool paused) { m_paused = paused; }
    bool IsPaused() const { return m_paused; }

    // 设置全局时间缩放
    void SetTimeScale(float scale) { m_timeScale = scale; }
    float GetTimeScale() const { return m_timeScale; }

private:
    // 位置哈希键
    uint64_t MakeKey(int x, int y) const {
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint32_t>(y);
    }

    // 动画瓦片映射
    std::unordered_map<uint64_t, AnimatedTileState> m_animatedTiles;

    // 本帧变化的瓦片
    std::vector<TileChange> m_changedTiles;

    // 暂停状态
    bool m_paused = false;

    // 时间缩放
    float m_timeScale = 1.0f;
};

} // namespace PrismaEngine
