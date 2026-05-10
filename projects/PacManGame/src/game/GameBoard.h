#pragma once

#include "../core/GameConstants.h"
#include <vector>
#include <array>
#include <memory>

namespace Prisma {
    namespace Graphic {
        class RenderCommandContext;
    }
}

namespace PacMan {

class PacMan;
class Ghost;
class Pellet;
class PowerPellet;

/**
 * @brief 游戏棋盘
 * 管理游戏地图、碰撞检测、豆子等
 */
class GameBoard {
public:
    GameBoard();
    ~GameBoard() = default;

    // ========== 初始化 ==========

    /**
     * @brief 加载地图
     * @param mapData 地图数据（二维数组）
     */
    void LoadMap(const std::vector<std::vector<TileType>>& mapData);

    /**
     * @brief 加载默认地图
     */
    void LoadDefaultMap();

    /**
     * @brief 重置地图
     */
    void Reset();

    // ========== 方块访问 ==========

    /**
     * @brief 获取方块类型
     * @param x X 坐标（格子坐标）
     * @param y Y 坐标（格子坐标）
     */
    TileType GetTile(int x, int y) const;

    /**
     * @brief 设置方块类型
     */
    void SetTile(int x, int y, TileType type);

    /**
     * @brief 检查位置是否有效
     */
    bool IsValidPosition(int x, int y) const;

    // ========== 碰撞检测 ==========

    /**
     * @brief 检查格子是否可通行
     */
    bool IsWalkable(int x, int y) const;

    /**
     * @brief 检查格子是否是墙
     */
    bool IsWall(int x, int y) const;

    /**
     * @brief 检查格子是豆子
     */
    bool IsPellet(int x, int y) const;

    /**
     * @brief 检查格子是能量药丸
     */
    bool IsPowerPellet(int x, int y) const;

    // ========== 豆子管理 ==========

    /**
     * @brief 吃掉豆子
     * @return 返回得分
     */
    int EatPellet(int x, int y);

    /**
     * @brief 吃掉能量药丸
     */
    int EatPowerPellet(int x, int y);

    /**
     * @brief 获取剩余豆子数量
     */
    int GetRemainingPellets() const { return m_remainingPellets; }
    int GetTotalPellets() const { return m_totalPellets; }

    /**
     * @brief 检查是否所有豆子都被吃掉
     */
    bool IsAllPelletsEaten() const { return m_remainingPellets == 0; }

    /**
     * @brief 渲染游戏板
     */
    void Render();

    // ========== 生成点 ==========

    /**
     * @brief 获取吃豆人生成点
     */
    glm::ivec2 GetPacmanSpawnPoint() const { return m_pacmanSpawnPoint; }

    /**
     * @brief 获取幽灵生成点
     */
    const std::vector<glm::ivec2>& GetGhostSpawnPoints() const { return m_ghostSpawnPoints; }

    // ========== 尺寸 ==========

    /**
     * @brief 获取地图宽度（格子数）
     */
    int GetWidth() const { return m_width; }

    /**
     * @brief 获取地图高度（格子数）
     */
    int GetHeight() const { return m_height; }

    // ========== 像素坐标转换 ==========

    /**
     * @brief 格子坐标转像素坐标（中心点）
     */
    glm::vec2 GridToPixel(int x, int y) const;

    /**
     * @brief 像素坐标转格子坐标
     */
    glm::ivec2 PixelToGrid(const glm::vec2& position) const;

    /**
     * @brief 格子坐标转像素坐标（左上角）
     */
    glm::vec2 GridToPixelCorner(int x, int y) const;

    // ========== 穿墙（隧道） ==========

    /**
     * @brief 检查是否需要穿墙
     * @param position 当前位置
     * @param outPosition 输出穿墙后的位置
     * @return 是否穿墙
     */
    bool CheckTunnel(const glm::vec2& position, glm::vec2& outPosition);

    // ========== 地图数据 ==========

    /**
     * @brief 获取地图数据
     */
    const std::vector<std::vector<TileType>>& GetMapData() const { return m_mapData; }

private:
    std::vector<std::vector<TileType>> m_mapData;
    int m_width = BOARD_WIDTH;
    int m_height = BOARD_HEIGHT;
    int m_totalPellets = 0;
    int m_remainingPellets = 0;

    glm::ivec2 m_pacmanSpawnPoint{13, 23};
    std::vector<glm::ivec2> m_ghostSpawnPoints;

    // 隧道位置
    int m_tunnelXLeft = 0;
    int m_tunnelXRight = BOARD_WIDTH - 1;
    int m_tunnelY = 14;
};

} //内 namespace PacMan
