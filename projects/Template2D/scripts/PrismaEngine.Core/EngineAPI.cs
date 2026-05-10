using System;
using System.Runtime.InteropServices;

namespace PrismaEngine;

/// <summary>
/// C++ 引擎通过 Bootstrap 传入的 C API 函数指针表。
/// 结构与 C++ 端 PrismaAPI 一一对应。
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct PrismaAPI
{
    // Logging
    public delegate* unmanaged<byte*, byte*, void> Log;

    // Entity lifecycle
    public delegate* unmanaged<uint> CreateEntity;
    public delegate* unmanaged<uint, void> DestroyEntity;

    // Transform
    public delegate* unmanaged<uint, float, float, void> SetPosition;
    public delegate* unmanaged<uint, float*, float*, void> GetPosition;
    public delegate* unmanaged<uint, float, void> SetRotation;
    public delegate* unmanaged<uint, float> GetRotation;
    public delegate* unmanaged<uint, float, float, void> SetScale;
    public delegate* unmanaged<uint, float*, float*, void> GetScale;

    // Sprite renderer
    public delegate* unmanaged<uint, float, float, float, float, void> SetColor;
    public delegate* unmanaged<uint, float*, float*, float*, float*, void> GetColor;
    public delegate* unmanaged<uint, float, float, void> SetSize;
    public delegate* unmanaged<uint, float*, float*, void> GetSize;

    // Input
    public delegate* unmanaged<int, bool> IsKeyDown;
    public delegate* unmanaged<float> GetMouseX;
    public delegate* unmanaged<float> GetMouseY;

    // Time
    public delegate* unmanaged<float> GetDeltaTime;

    // Camera
    public delegate* unmanaged<float, float, void> SetCameraPos;
    public delegate* unmanaged<float*, float*, void> GetCameraPos;
}

/// <summary>
/// 内部引擎 API 绑定。C# 核心库通过此结构调用 C++ 引擎。
/// </summary>
internal static unsafe class NativeAPI
{
    internal static PrismaAPI API;

    internal static void Init(PrismaAPI* api)
    {
        API = *api;
    }
}
