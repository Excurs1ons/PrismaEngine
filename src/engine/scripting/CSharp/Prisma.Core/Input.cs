using System;
using System.Runtime.CompilerServices;

namespace Prisma;

/// <summary>
/// 输入上下文信息�?
/// Input context information.
/// </summary>
public class InputContext
{
    // Cherno Optimization: 预计算哈希值，干掉热路径上的字符串比较�?
    public static readonly uint HorizontalHash = HashString("Horizontal");
    public static readonly uint VerticalHash = HashString("Vertical");

    private readonly bool[] _keyStates = new bool[512];
    private float _mouseX;
    private float _mouseY;
    private float _mouseDeltaX;
    private float _mouseDeltaY;
    private float _mouseScrollX;
    private float _mouseScrollY;

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool GetKey(KeyCode key) => (uint)key < (uint)_keyStates.Length && _keyStates[(int)key];

    public float MouseX => _mouseX;
    public float MouseY => _mouseY;
    public Vector2 MousePosition => new(_mouseX, _mouseY);
    public float MouseDeltaX => _mouseDeltaX;
    public float MouseDeltaY => _mouseDeltaY;
    public Vector2 MouseDelta => new(_mouseDeltaX, _mouseDeltaY);
    public float MouseScrollX => _mouseScrollX;
    public float MouseScrollY => _mouseScrollY;
    public Vector2 MouseScroll => new(_mouseScrollX, _mouseScrollY);

    internal unsafe void UpdateState()
    {
        for (int i = 0; i < _keyStates.Length; i++) _keyStates[i] = Interop.API.IsKeyDown(i);
        _mouseX = Interop.API.GetMouseX();
        _mouseY = Interop.API.GetMouseY();
        _mouseDeltaX = Interop.API.GetMouseDeltaX();
        _mouseDeltaY = Interop.API.GetMouseDeltaY();
        _mouseScrollX = Interop.API.GetMouseScrollX();
        _mouseScrollY = Interop.API.GetMouseScrollY();
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public float GetAxis(string axisName) => GetAxis(HashString(axisName));

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public float GetAxis(uint axisHash)
    {
        if (axisHash == HorizontalHash)
        {
            float val = 0;
            if (GetKey(KeyCode.D) || GetKey(KeyCode.Right)) val += 1;
            if (GetKey(KeyCode.A) || GetKey(KeyCode.Left)) val -= 1;
            return val;
        }
        if (axisHash == VerticalHash)
        {
            float val = 0;
            if (GetKey(KeyCode.W) || GetKey(KeyCode.Up)) val += 1;
            if (GetKey(KeyCode.S) || GetKey(KeyCode.Down)) val -= 1;
            return val;
        }
        return 0;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static uint HashString(string s)
    {
        uint hash = 2166136261;
        foreach (char c in s) hash = (hash ^ c) * 16777619;
        return hash;
    }
}

public static class Input
{
    public static bool GetKey(KeyCode key) => World.Active?.Input.GetKey(key) ?? false;
    public static float GetAxis(string axisName) => World.Active?.Input.GetAxis(axisName) ?? 0;
    public static float GetAxis(uint axisHash) => World.Active?.Input.GetAxis(axisHash) ?? 0;
    public static Vector2 MousePosition => World.Active?.Input.MousePosition ?? Vector2.Zero;
    public static float MouseDeltaX => World.Active?.Input.MouseDeltaX ?? 0;
    public static float MouseDeltaY => World.Active?.Input.MouseDeltaY ?? 0;
    public static Vector2 MouseDelta => World.Active?.Input.MouseDelta ?? Vector2.Zero;
    public static float MouseScrollX => World.Active?.Input.MouseScrollX ?? 0;
    public static float MouseScrollY => World.Active?.Input.MouseScrollY ?? 0;
    public static Vector2 MouseScroll => World.Active?.Input.MouseScroll ?? Vector2.Zero;
}
