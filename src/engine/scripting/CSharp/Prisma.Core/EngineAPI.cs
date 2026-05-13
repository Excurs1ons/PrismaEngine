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
    // 以下字段?C++ PrismaAPI 对齐（C# 当前未直接调用，但必须占位保证偏移正确）
    public delegate* unmanaged<float, float, void> SetCameraPos;
    public delegate* unmanaged<float*, float*, void> GetCameraPos;

    // Gizmos
    public delegate* unmanaged<float, float, float, float, float, float, float, float, void> DrawGizmoLine;
    public delegate* unmanaged<float, float, float, float, float, float, float, float, void> DrawGizmoRect;
    public delegate* unmanaged<byte*, float, float, float, float, float, float, float, void> DrawGizmoString;

    public delegate* unmanaged<uint> GetEntityCapacity;

    // 2D 光照 API
    public delegate* unmanaged<int, uint> CreateLight;
    public delegate* unmanaged<uint, void> DestroyLight;
    public delegate* unmanaged<uint, float, float, void> SetLightPos;
    public delegate* unmanaged<uint, float, float, float, void> SetLightColor;
    public delegate* unmanaged<uint, float, void> SetLightIntensity;
    public delegate* unmanaged<uint, float, void> SetLightRadius;
    public delegate* unmanaged<uint, float, void> SetLightFalloff;
    public delegate* unmanaged<uint, int, void> SetLightOrder;
    public delegate* unmanaged<uint, int, void> SetLightBlendMode;

    // 2D 环境光
    public delegate* unmanaged<float, float, float, void> SetAmbientLight;

    // [诊断] C++ 侧在 Initialize 中设为 sizeof(PrismaAPI)，C# 侧在 Init 中校验
    public uint StructSize;
}

internal static unsafe class NativeAPI
{
    internal static PrismaAPI API;

    // 双缓冲区指针（每帧由 SyncBufferPointers 更新）
    internal static TransformBufferSoA* TransformBuffer_Read;
    internal static TransformBufferSoA* TransformBuffer_Write;
    internal static RenderBufferSoA* RenderBuffer;

    internal static void Init(PrismaAPI* api)
    {
        // [诊断] 校验 C++/C# PrismaAPI 结构体大小一致性
        uint expectedSize = (uint)sizeof(PrismaAPI);
        if (api->StructSize != 0 && api->StructSize != expectedSize)
        {
            var msg = $"PrismaAPI 结构体版本不匹配！\n" +
                      $"  C++ sizeof(PrismaAPI) = {api->StructSize}\n" +
                      $"  C#  sizeof(PrismaAPI) = {expectedSize}\n" +
                      $"  请确保 C++ ScriptEngine.h 与 C# EngineAPI.cs 字段完全一致，\n" +
                      $"  并重新编译 Prisma.Core.dll 与 GameScripts.dll。";
            throw new InvalidOperationException(msg);
        }

        API = *api;
        // 初始指针基于当前 m_writeIndex(=0)：
        //   GetB = GetRead = &layoutB  (C# 从此读)
        //   GetA = GetWrite = &layoutA (C# 往此写)
        TransformBuffer_Read = API.GetTransformBufferB();
        TransformBuffer_Write = API.GetTransformBufferA();
        RenderBuffer = API.GetRenderBuffer();
    }

    /// <summary>
    /// 每帧同步 C# 侧的双缓冲指针，使其与 C++ m_writeIndex 保持一致。
    /// C++ 是唯一的交换权威，每次 SwapBuffers 后 Read/Write 互换。
    /// </summary>
    internal static void SyncBufferPointers()
    {
        unsafe
        {
            TransformBuffer_Read = API.GetTransformBufferB();
            TransformBuffer_Write = API.GetTransformBufferA();
        }
    }
}
