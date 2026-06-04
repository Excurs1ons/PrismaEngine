using System;
using Prisma;

namespace GameScripts;

/// <summary>
/// 房间定义：位置 + 尺寸
/// </summary>
public struct RoomDef
{
    public float X, Y, Width, Height;
    public string Name;
}

/// <summary>
/// 房间切换管理器
/// - 跟踪玩家当前所在房间
/// - 触发进入/离开事件
/// - 支持房间过渡动画（淡入淡出效果通过逐步改变透明度实现）
/// </summary>
public static class RoomManager
{
    // 房间定义（水平方向 3 个房间，每个 640×480）
    public static readonly RoomDef[] Rooms = new RoomDef[]
    {
        new RoomDef { X = 0,    Y = 0, Width = 640,  Height = 480, Name = "Room 1" },
        new RoomDef { X = 640,  Y = 0, Width = 640,  Height = 480, Name = "Room 2" },
        new RoomDef { X = 1280, Y = 0, Width = 640,  Height = 480, Name = "Room 3" },
    };

    public static int CurrentRoomIndex { get; private set; } = 0;
    public static bool IsTransitioning { get; private set; } = false;
    public static float TransitionTimer { get; private set; } = 0f;
    private const float TransitionDuration = 0.3f;

    /// <summary>
    /// 每帧检测玩家位置，返回当前所在房间索引（-1 = 在房间外）
    /// </summary>
    public static int GetRoomAtPosition(float x, float y)
    {
        for (int i = 0; i < Rooms.Length; i++)
        {
            var r = Rooms[i];
            if (x >= r.X && x < r.X + r.Width &&
                y >= r.Y && y < r.Y + r.Height)
                return i;
        }
        return -1;
    }

    /// <summary>
    /// 每帧更新，处理房间切换
    /// </summary>
    public static void Update(TimeContext time, InputContext input)
    {
        Node player = GameState.PlayerNode;
        if (player.Handle == 0) return;

        int newRoom = GetRoomAtPosition(player.X, player.Y);

        if (IsTransitioning)
        {
            TransitionTimer -= time.DeltaTime;
            if (TransitionTimer <= 0f)
            {
                IsTransitioning = false;
                CurrentRoomIndex = newRoom >= 0 ? newRoom : CurrentRoomIndex;
            }
            return;
        }

        if (newRoom >= 0 && newRoom != CurrentRoomIndex)
        {
            int prevRoom = CurrentRoomIndex;
            CurrentRoomIndex = newRoom;
            IsTransitioning = true;
            TransitionTimer = TransitionDuration;

            Console.WriteLine($"[RoomManager] Transition: {Rooms[prevRoom].Name} -> {Rooms[newRoom].Name}");
        }
    }

    /// <summary>
    /// 获取当前房间的边界（用于相机锁定）
    /// </summary>
    public static void GetCurrentRoomBounds(out float x, out float y, out float w, out float h)
    {
        int idx = CurrentRoomIndex;
        if (idx < 0 || idx >= Rooms.Length)
        {
            x = y = 0; w = 640; h = 480;
            return;
        }
        var r = Rooms[idx];
        x = r.X; y = r.Y; w = r.Width; h = r.Height;
    }
}
