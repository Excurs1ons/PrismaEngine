using System;
using System.Runtime.InteropServices;

namespace Prisma;

/// <summary>
/// 热数据：物理与变�?(动�?SoA 布局，指针指�?C++ vector.data())�?
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public unsafe struct TransformBufferSoA
{
    public float* PosX;
    public float* PosY;
    public float* Rotation;
    public float* ScaleX;
    public float* ScaleY;
}

/// <summary>
/// 冷数据：渲染与外�?(动�?SoA 布局)�?
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public unsafe struct RenderBufferSoA
{
    public uint*   Active;
    public uint*   Generation;
    public float*  ColorR;
    public float*  ColorG;
    public float*  ColorB;
    public float*  ColorA;
    public float*  SizeW;
    public float*  SizeH;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct PrismaAPI
{
    public delegate* unmanaged<byte*, byte*, void> Log;
    public delegate* unmanaged<uint> CreateEntity;
    public delegate* unmanaged<uint, void> DestroyEntity;
    public delegate* unmanaged<TransformBufferSoA*> GetTransformBufferA;
    public delegate* unmanaged<TransformBufferSoA*> GetTransformBufferB;
    public delegate* unmanaged<RenderBufferSoA*> GetRenderBuffer;
    public delegate* unmanaged<int, bool> IsKeyDown;
    public delegate* unmanaged<float> GetMouseX;
    public delegate* unmanaged<float> GetMouseY;
    public delegate* unmanaged<float> GetDeltaTime;
    // 以下字段�?C++ PrismaAPI 对齐（C# 当前未直接调用，但必须占位保证偏移正确）
    public delegate* unmanaged<float, float, void> SetCameraPos;
    public delegate* unmanaged<float*, float*, void> GetCameraPos;
    public delegate* unmanaged<uint> GetEntityCapacity;
}

internal static unsafe class NativeAPI
{
    internal static PrismaAPI API;

    // 双缓冲区指针
    internal static TransformBufferSoA* TransformBuffer_Read;
    internal static TransformBufferSoA* TransformBuffer_Write;
    internal static RenderBufferSoA* RenderBuffer;

    internal static void Init(PrismaAPI* api)
    {
        API = *api;
        TransformBuffer_Read = API.GetTransformBufferA();
        TransformBuffer_Write = API.GetTransformBufferB();
        RenderBuffer = API.GetRenderBuffer();
    }

    /// <summary> 交换读写缓冲区�?</summary>
    public static void SwapBuffers()
    {
        var temp = TransformBuffer_Read;
        TransformBuffer_Read = TransformBuffer_Write;
        TransformBuffer_Write = temp;
    }
}
