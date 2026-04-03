#include "GameBoard.h"
#include "graphic/Renderer2D.h"
#include <algorithm>

namespace PacMan {

// ========== 经典迷宫地图数据 ==========

// 0=Empty, 1=Wall, 2=Pellet, 3=PowerPellet, 4=GhostHouse, 5=PacmanSpawn
static const int CLASSIC_MAZE[BOARD_HEIGHT][BOARD_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,4,4,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,3,2,2,2,2,2,2,2,2,2,2,2,5,2,2,2,2,2,2,2,2,2,2,2,2,3,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// ========== GameBoard 实现 ==========

GameBoard::GameBoard()
    : m_width(BOARD_WIDTH)
    , m_height(BOARD_HEIGHT)
    , m_totalPellets(0)
    , m_remainingPellets(0)
    , m_tunnelXLeft(0)
    , m_tunnelXRight(BOARD_WIDTH - 1)
    , m_tunnelY(14)
{
    LoadDefaultMap();
}

void GameBoard::LoadMap(const std::vector<std::vector<TileType>>& mapData) {
    m_mapData = mapData;
    m_height = static_cast<int>(mapData.size());
    m_width = static_cast<int>(mapData[0].size());

    // 计算豆子总数
    m_totalPellets = 0;
    m_remainingPellets = 0;
    m_ghostSpawnPoints.clear();

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            TileType tile = GetTile(x, y);
            if (tile == TileType::Pellet || tile == TileType::PowerPellet) {
                m_totalPellets++;
                m_remainingPellets++;
            } else if (tile == TileType::PacmanSpawn) {
                m_pacmanSpawnPoint = glm::ivec2(x, y);
            } else if (tile == TileType::GhostHouse) {
                m_ghostSpawnPoints.push_back(glm::ivec2(x, y));
            }
        }
    }
}

void GameBoard::LoadDefaultMap() {
    m_mapData.resize(m_height);
    m_totalPellets = 0;
    m_remainingPellets = 0;
    m_ghostSpawnPoints.clear();

    for (int y = 0; y < m_height; y++) {
        m_mapData[y].resize(m_width);
        for (int x = 0; x < m_width; x++) {
            int rawType = CLASSIC_MAZE[y][x];
            m_mapData[y][x] = static_cast<TileType>(rawType);

            if (rawType == 2 || rawType == 3) {
                m_totalPellets++;
                m_remainingPellets++;
            } else if (rawType == 5) {
                m_pacmanSpawnPoint = glm::ivec2(x, y);
            } else if (rawType == 4) {
                m_ghostSpawnPoints.push_back(glm::ivec2(x, y));
            }
        }
    }
}

void GameBoard::Reset() {
    LoadDefaultMap();
}

TileType GameBoard::GetTile(int x, int y) const {
    if (!IsValidPosition(x, y)) {
        return TileType::Wall;  // 越界视为墙
    }
    return m_mapData[y][x];
}

void GameBoard::SetTile(int x, int y, TileType type) {
    if (IsValidPosition(x, y)) {
        m_mapData[y][x] = type;
    }
}

bool GameBoard::IsValidPosition(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

bool GameBoard::IsWalkable(int x, int y) const {
    TileType tile = GetTile(x, y);
    return tile != TileType::Wall;
}

bool GameBoard::IsWall(int x, int y) const {
    return GetTile(x, y) == TileType::Wall;
}

bool GameBoard::IsPellet(int x, int y) const {
    return GetTile(x, y) == TileType::Pellet;
}

bool GameBoard::IsPowerPellet(int x, int y) const {
    return GetTile(x, y) == TileType::PowerPellet;
}

void GameBoard::Render() {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            TileType tile = GetTile(x, y);
            glm::vec2 pos = GridToPixelCorner(x, y);
            
            if (tile == TileType::Wall) {
                Prisma::Graphic::Renderer2D::DrawQuad(pos, glm::vec2(TILE_SIZE, TILE_SIZE), Prisma::Color(0.0f, 0.0f, 1.0f, 1.0f));
            } else if (tile == TileType::Pellet) {
                glm::vec2 pelletPos = pos + glm::vec2(TILE_SIZE * 0.4f);
                Prisma::Graphic::Renderer2D::DrawQuad(pelletPos, glm::vec2(TILE_SIZE * 0.2f), Prisma::Color(1.0f, 1.0f, 1.0f, 1.0f));
            } else if (tile == TileType::PowerPellet) {
                glm::vec2 pelletPos = pos + glm::vec2(TILE_SIZE * 0.3f);
                Prisma::Graphic::Renderer2D::DrawQuad(pelletPos, glm::vec2(TILE_SIZE * 0.4f), Prisma::Color(1.0f, 1.0f, 1.0f, 1.0f));
            }
        }
    }
}

int GameBoard::EatPellet(int x, int y) {
    if (IsPellet(x, y)) {
        SetTile(x, y, TileType::Empty);
        m_remainingPellets--;
        return PELLET_SCORE;
    }
    return 0;
}

int GameBoard::EatPowerPellet(int x, int y) {
    if (IsPowerPellet(x, y)) {
        SetTile(x, y, TileType::Empty);
        m_remainingPellets--;
        return POWER_PELLET_SCORE;
    }
    return 0;
}

glm::vec2 GameBoard::GridToPixel(int x, int y) const {
    return glm::vec2(
        static_cast<float>(x * TILE_SIZE) + TILE_SIZE / 2.0f,
        static_cast<float>(y * TILE_SIZE) + TILE_SIZE / 2.0f
    );
}

glm::vec2 GameBoard::GridToPixelCorner(int x, int y) const {
    return glm::vec2(
        static_cast<float>(x * TILE_SIZE),
        static_cast<float>(y * TILE_SIZE)
    );
}

glm::ivec2 GameBoard::PixelToGrid(const glm::vec2& position) const {
    return glm::ivec2(
        static_cast<int>(position.x / TILE_SIZE),
        static_cast<int>(position.y / TILE_SIZE)
    );
}

bool GameBoard::CheckTunnel(const glm::vec2& position, glm::vec2& outPosition) {
    glm::ivec2 gridPos = PixelToGrid(position);

    // 检查是否在隧道 Y 坐标
    if (gridPos.y == m_tunnelY) {
        // 从左边出去
        if (position.x < 0) {
            outPosition = position + glm::vec2(m_width * TILE_SIZE, 0.0f);
            return true;
        }
        // 从右边出去
        else if (position.x >= m_width * TILE_SIZE) {
            outPosition = position - glm::vec2(m_width * TILE_SIZE, 0.0f);
            return true;
        }
    }

    return false;
}

} // namespace PacMan
