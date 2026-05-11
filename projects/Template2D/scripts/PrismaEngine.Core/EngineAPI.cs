using System;
using System.Runtime.InteropServices;

namespace PrismaEngine;

/// <summary>
/// 热数据：物理与变换 (SoA 布局)。
/// Hot Data: Transform and Physics (SoA).
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public unsafe struct TransformBufferSoA
{
    public fixed float PosX[NativeAPI.MaxEntities];
    public fixed float PosY[NativeAPI.MaxEntities];
    public fixed float Rotation[NativeAPI.MaxEntities];
    public fixed float ScaleX[NativeAPI.MaxEntities];
    public fixed float ScaleY[NativeAPI.MaxEntities];
}

/// <summary>
/// 冷数据：渲染与外观 (SoA 布局)。
/// Cold Data: Rendering and Appearance (SoA).
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public unsafe struct RenderBufferSoA
{
    public fixed uint Active[NativeAPI.MaxEntities];
    public fixed uint Generation[NativeAPI.MaxEntities];
    public fixed float ColorR[NativeAPI.MaxEntities];
    public fixed float ColorG[NativeAPI.MaxEntities];
    public fixed float ColorB[NativeAPI.MaxEntities];
    public fixed float ColorA[NativeAPI.MaxEntities];
    public fixed float SizeW[NativeAPI.MaxEntities];
    public fixed float SizeH[NativeAPI.MaxEntities];
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct PrismaAPI
{
    public delegate* unmanaged<byte*, byte*, void> Log;
    public delegate* unmanaged<uint> CreateEntity;
    public delegate* unmanaged<uint, void> DestroyEntity;
    // Cherno: 获取两个变换缓冲区以实现双缓冲
    public delegate* unmanaged<TransformBufferSoA*> GetTransformBufferA;
    public delegate* unmanaged<TransformBufferSoA*> GetTransformBufferB;
    public delegate* unmanaged<RenderBufferSoA*> GetRenderBuffer;
    public delegate* unmanaged<int, bool> IsKeyDown;
    public delegate* unmanaged<float> GetMouseX;
    public delegate* unmanaged<float> GetMouseY;
    public delegate* unmanaged<float> GetDeltaTime;
}

internal static unsafe class NativeAPI
{
    public const int MaxEntities = 32768; // 翻倍，继续向百万迈进
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

    /// <summary> 交换读写缓冲区。 </summary>
    public static void SwapBuffers()
    {
        var temp = TransformBuffer_Read;
        TransformBuffer_Read = TransformBuffer_Write;
        TransformBuffer_Write = temp;
    }
}
