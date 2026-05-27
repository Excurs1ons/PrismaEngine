using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class TilemapCollider : Script
{
    const float TileSize = 16f;
    const int CollisionLayer = 0;

    public string TilemapPath = "assets/maps/test_dungeon.json";

    public override void OnCreate()
    {
        LoadTilemap();
    }

    unsafe void LoadTilemap()
    {
        if (!string.IsNullOrEmpty(TilemapPath))
        {
            var pathUtf8 = Interop.StringToUtf8(TilemapPath);
            fixed (byte* p = pathUtf8)
            {
                uint handle = Interop.API.Tilemap_Load(p);
                GameState.TilemapHandle = handle;
                if (handle != 0)
                {
                    RebuildCollisionData(handle);
                    Debug.Log("[TilemapCollider] Loaded: " + TilemapPath
                        + " W:" + Interop.API.Tilemap_GetWidth(handle)
                        + " H:" + Interop.API.Tilemap_GetHeight(handle));
                }
                else
                {
                    Debug.LogError("[TilemapCollider] Failed to load: " + TilemapPath);
                }
            }
        }
    }

    unsafe void RebuildCollisionData(uint handle)
    {
        uint width = Interop.API.Tilemap_GetWidth(handle);
        uint height = Interop.API.Tilemap_GetHeight(handle);
        int maxTiles = (int)(width * height);

        if (maxTiles <= 0) return;

        // Pre-allocate max possible size
        var temp = new float[maxTiles * 4];
        int count = 0;

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                if (Interop.API.Tilemap_IsSolid(handle, CollisionLayer, x, y) != 0)
                {
                    int idx = count * 4;
                    temp[idx + 0] = x * TileSize;
                    temp[idx + 1] = y * TileSize;
                    temp[idx + 2] = (x + 1) * TileSize;
                    temp[idx + 3] = (y + 1) * TileSize;
                    count++;
                }
            }
        }

        if (count > 0)
        {
            GameState.SolidTileData = new float[count * 4];
            Array.Copy(temp, GameState.SolidTileData, count * 4);
            GameState.SolidTileCount = count;
        }
        else
        {
            GameState.SolidTileData = Array.Empty<float>();
            GameState.SolidTileCount = 0;
        }

        Debug.Log("[TilemapCollider] Solid tiles: " + count);
    }

    public override void OnDestroy()
    {
        if (GameState.TilemapHandle != 0)
        {
            unsafe { Interop.API.Tilemap_Unload(GameState.TilemapHandle); }
            GameState.TilemapHandle = 0;
        }
    }
}
