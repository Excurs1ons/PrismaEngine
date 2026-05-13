using System;
using System.Text;

namespace Prisma;

public static class Gizmos
{
    public static Color Color { get; set; } = Color.White;

    public static void DrawLine(Vector2 start, Vector2 end)
    {
        DrawLine(start, end, Color);
    }

    public static unsafe void DrawLine(Vector2 start, Vector2 end, Color color)
    {
        NativeAPI.API.DrawGizmoLine(start.X, start.Y, end.X, end.Y, color.R, color.G, color.B, color.A);
    }

    public static void DrawRect(Vector2 position, Vector2 size)
    {
        DrawRect(position, size, Color);
    }

    public static unsafe void DrawRect(Vector2 position, Vector2 size, Color color)
    {
        NativeAPI.API.DrawGizmoRect(position.X, position.Y, size.X, size.Y, color.R, color.G, color.B, color.A);
    }

    public static void DrawString(string text, Vector2 position, float scale = 1.0f)
    {
        DrawString(text, position, scale, Color);
    }

    public static unsafe void DrawString(string text, Vector2 position, float scale, Color color)
    {
        if (string.IsNullOrEmpty(text)) return;
        
        byte[] bytes = Encoding.UTF8.GetBytes(text + "\0");
        fixed (byte* ptr = bytes)
        {
            NativeAPI.API.DrawGizmoString(ptr, position.X, position.Y, scale, color.R, color.G, color.B, color.A);
        }
    }
}
