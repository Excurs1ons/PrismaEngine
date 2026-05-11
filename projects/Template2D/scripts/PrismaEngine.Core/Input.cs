namespace PrismaEngine;

/// <summary>
/// 输入静态 API。
/// </summary>
public static class Input
{
    /// <summary>指定按键是否被按下。</summary>
    public static unsafe bool GetKey(KeyCode key)
        => NativeAPI.API.IsKeyDown((int)key);

    /// <summary>当前鼠标 X 坐标。</summary>
    public static unsafe float MouseX => NativeAPI.API.GetMouseX();

    /// <summary>当前鼠标 Y 坐标。</summary>
    public static unsafe float MouseY => NativeAPI.API.GetMouseY();

    /// <summary>当前鼠标位置。</summary>
    public static Vector2 MousePosition
    {
        get
        {
            unsafe
            {
                return new Vector2(NativeAPI.API.GetMouseX(), NativeAPI.API.GetMouseY());
            }
        }
    }
}
