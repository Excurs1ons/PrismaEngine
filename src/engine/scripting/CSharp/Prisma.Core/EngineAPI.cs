using System;
using System.Runtime.InteropServices;

namespace Prisma;

/// <summary>
/// Transform 数据布局（双缓冲）。内存布局与 C++ Prisma::TransformDataLayout 一致。
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct TransformDataLayout
{
    public float* PosX;
    public float* PosY;
    public float* Rotation;
    public float* ScaleX;
    public float* ScaleY;
}

/// <summary>
/// Render 数据布局（单缓冲）。内存布局与 C++ Prisma::RenderDataLayout 一致。
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct RenderDataLayout
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
    public delegate* unmanaged<TransformDataLayout*> GetTransformA;
    public delegate* unmanaged<TransformDataLayout*> GetTransformB;
    public delegate* unmanaged<RenderDataLayout*> GetRenderData;
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

    // ===== SRP Graphics API =====

    // Shader
    public delegate* unmanaged<byte*, uint, uint, uint> SrpCreateShader;
    public delegate* unmanaged<uint, void> SrpDestroyShader;

    // Pipeline
    public delegate* unmanaged<SRPPipelineDesc*, uint> SrpCreatePipeline;
    public delegate* unmanaged<uint, void> SrpDestroyPipeline;

    // Render Target
    public delegate* unmanaged<int, int, uint, int, uint> SrpCreateRenderTarget;
    public delegate* unmanaged<int, int, uint, uint> SrpCreateDepthTarget;
    public delegate* unmanaged<uint, void> SrpDestroyRenderTarget;
    public delegate* unmanaged<uint, void> SrpDestroyDepthTarget;

    // Buffers
    public delegate* unmanaged<void*, uint, uint, uint> SrpCreateVertexBuffer;
    public delegate* unmanaged<void*, uint, int, uint> SrpCreateIndexBuffer;
    public delegate* unmanaged<uint, void> SrpDestroyBuffer;

    // Texture
    public delegate* unmanaged<int, int, uint, void*, uint, uint> SrpCreateTexture2D;
    public delegate* unmanaged<uint, void> SrpDestroyTexture;

    // Sampler
    public delegate* unmanaged<SRPSamplerDesc*, uint> SrpCreateSampler;
    public delegate* unmanaged<uint, void> SrpDestroySampler;

	// Texture binding
	public delegate* unmanaged<uint, uint, uint, void> SrpCmdBindTexture;
	public delegate* unmanaged<uint, void> SrpCmdBlitRenderTarget;

	// Frame
    public delegate* unmanaged<void> SrpBeginFrame;
    public delegate* unmanaged<void> SrpEndFrame;

    // Command buffer (called between BeginFrame/EndFrame)
    public delegate* unmanaged<uint, uint*, uint, float*, float, int, int, void> SrpCmdBeginRenderPass;
    public delegate* unmanaged<void> SrpCmdEndRenderPass;
    public delegate* unmanaged<uint, void> SrpCmdBindPipeline;
    public delegate* unmanaged<uint, uint, uint, void> SrpCmdBindVertexBuffer;
    public delegate* unmanaged<uint, uint, int, void> SrpCmdBindIndexBuffer;
    public delegate* unmanaged<int, int, int, int, void> SrpCmdSetViewport;
    public delegate* unmanaged<int, int, int, int, void> SrpCmdSetScissor;
    public delegate* unmanaged<uint, uint, void*, void> SrpCmdPushConstants;
    public delegate* unmanaged<uint, uint, uint, void> SrpCmdDraw;
    public delegate* unmanaged<uint, uint, uint, int, void> SrpCmdDrawIndexed;
    public delegate* unmanaged<void> SrpCmdDrawFullScreenQuad;

    // Compute pipeline
    public delegate* unmanaged<uint, uint, uint> SrpCreateComputePipeline;
    public delegate* unmanaged<uint, void> SrpDestroyComputePipeline;
    public delegate* unmanaged<uint, void> SrpCmdBindComputePipeline;
    public delegate* unmanaged<uint, uint, uint, void> SrpCmdDispatch;
    public delegate* unmanaged<uint, uint, uint, void> SrpCmdBindComputeTexture;
    public delegate* unmanaged<uint, uint, void> SrpCmdBindStorageImage;
    public delegate* unmanaged<uint, uint, void> SrpCmdBindStorageBuffer;

    public delegate* unmanaged<void> SrpShutdown;

    // ===== 3D Camera API =====
    public delegate* unmanaged<float, float, float, void> SetCamera3DPos;
    public delegate* unmanaged<float*, float*, float*, void> GetCamera3DPos;
    public delegate* unmanaged<float, float, void> SetCameraRotation;
    public delegate* unmanaged<float, float, float, void> MoveCameraLocal;

    // ===== Enhanced Input API =====
    public delegate* unmanaged<float> GetMouseDeltaX;
    public delegate* unmanaged<float> GetMouseDeltaY;
    public delegate* unmanaged<float> GetMouseScrollX;
    public delegate* unmanaged<float> GetMouseScrollY;
    public delegate* unmanaged<bool, void> SetMouseCapture;
    public delegate* unmanaged<int, bool> IsKeyJustPressed;

    // ===== Path Tracing Pipeline Control =====
    public delegate* unmanaged<uint, void> PtSetMaxSamples;
    public delegate* unmanaged<uint> PtGetFrameCount;
    public delegate* unmanaged<void> PtResetAccumulation;
    public delegate* unmanaged<bool, void> PtSetNEE;
    public delegate* unmanaged<bool> PtGetNEE;
    public delegate* unmanaged<void> PtCycleMode;
    public delegate* unmanaged<byte*, uint, void> PtGetModeName;
    public delegate* unmanaged<bool> PtIsConverged;
    public delegate* unmanaged<uint> PtGetMaxSamples;

    // ===== Audio API =====
    public delegate* unmanaged<bool> IsAudioInitialized;
    public delegate* unmanaged<float> AudioGetMasterVolume;
    public delegate* unmanaged<float, void> AudioSetMasterVolume;
    public delegate* unmanaged<byte*, AudioPlayDesc*, uint> AudioPlayClip;
    public delegate* unmanaged<uint, void> AudioStop;
    public delegate* unmanaged<void> AudioStopAll;
    public delegate* unmanaged<uint, float, void> AudioSetVolume;
    public delegate* unmanaged<uint, bool> AudioIsPlaying;

    // ===== AudioGraph API =====
    public delegate* unmanaged<uint, uint, ulong> AudioCreateGraph;
    public delegate* unmanaged<ulong, void> AudioDestroyGraph;
    public delegate* unmanaged<ulong, byte*, byte*, ulong> AudioGraphCreateNode;
    public delegate* unmanaged<ulong, ulong, void> AudioGraphRemoveNode;
    public delegate* unmanaged<ulong, ulong, byte*, ulong, byte*, bool> AudioGraphConnect;
    public delegate* unmanaged<ulong, ulong, ulong, bool> AudioGraphDisconnect;
    public delegate* unmanaged<ulong, byte*, float, void> AudioNodeSetParam;
    public delegate* unmanaged<ulong, byte*, float> AudioNodeGetParam;
    public delegate* unmanaged<ulong, byte*> AudioNodeGetName;
    public delegate* unmanaged<ulong, byte*, void> AudioNodeSetName;
    public delegate* unmanaged<ulong, void> AudioNodeDestroy;

    // ===== Level Meter + Spectrum Readback =====
    public delegate* unmanaged<ulong, uint, float*, float*, float*, float*, bool> AudioGetLevelMeterData;
    public delegate* unmanaged<uint, ulong> AudioCreateSpectrumAnalyzer;
    public delegate* unmanaged<ulong, void> AudioDestroySpectrumAnalyzer;
    public delegate* unmanaged<ulong, float*, uint, uint, void> AudioSpectrumProcessFloats;
    public delegate* unmanaged<ulong, float*, float*, float*, uint, uint> AudioSpectrumGetBins;
    public delegate* unmanaged<ulong, float> AudioSpectrumGetPeak;

    // ===== Physics2D =====
    public delegate* unmanaged<float, float, float, float, float, float, float, float, int> Physics2D_CheckAABB;
    public delegate* unmanaged<float, float, float, float, float*, float*, float*, int, int*, int*, int> Physics2D_ResolvePlatform;

    // ===== Tilemap =====
    public delegate* unmanaged<byte*, uint> Tilemap_Load;
    public delegate* unmanaged<uint, void> Tilemap_Unload;
    public delegate* unmanaged<uint, int, int, int, uint> Tilemap_GetTile;
    public delegate* unmanaged<uint, int, int, int, int> Tilemap_IsSolid;
    public delegate* unmanaged<uint, uint> Tilemap_GetWidth;
    public delegate* unmanaged<uint, uint> Tilemap_GetHeight;

    // ===== FileSystem / Asset IO =====
    public delegate* unmanaged<byte*, nuint*, void*> ReadAssetData;
    public delegate* unmanaged<void*, void> FreeAssetData;

    // ===== High-Level Audio API (BGM/SFX) =====
    public delegate* unmanaged<byte*, float, float, void> AudioPlaySFX;
    public delegate* unmanaged<byte*, float, bool, void> AudioPlayBGM;
    public delegate* unmanaged<float, void> AudioStopBGM;
    public delegate* unmanaged<float, void> AudioSetBGMVolume;
    public delegate* unmanaged<float, void> AudioSetSFXVolume;
    public delegate* unmanaged<float, float, void> AudioSetListenerPosition;

    // ===== AudioZone2D (2D Regional Audio) =====
    public delegate* unmanaged<float, float, float, float, byte*, float, bool, float, uint> AudioZoneRegister;
    public delegate* unmanaged<uint, void> AudioZoneUnregister;
    public delegate* unmanaged<float, float, void> AudioZoneSetPlayerPos;
    public delegate* unmanaged<byte*, float, void> AudioZonePlaySFX;

    public byte* ProjectName;

    // [诊断] C++ 侧在 Initialize 中设为 sizeof(PrismaAPI)，C# 侧在 Init 中校验
    public uint StructSize;

    // ===== UI Rendering API =====
    public delegate* unmanaged<float, float, float, float, float, float, float, float, void> UIDrawQuad;
    public delegate* unmanaged<byte*, float, float, float, float, float, float, float, void> UIDrawString;
    public delegate* unmanaged<byte*, float, float> UIGetStringWidth;

    // ===== SpriteAnimation C# Bindings =====
    public delegate* unmanaged<byte*, uint> SpriteAnimationCreate;
    public delegate* unmanaged<uint, float, float, float, float, float, void> SpriteAnimationAddFrame;
    public delegate* unmanaged<uint, byte*, bool, void> SpriteAnimationPlay;
    public delegate* unmanaged<uint, void> SpriteAnimationStop;
    public delegate* unmanaged<uint, void> SpriteAnimationPause;
    public delegate* unmanaged<uint, bool> SpriteAnimationIsPlaying;
    public delegate* unmanaged<uint, bool, void> SpriteAnimationSetLooping;
    public delegate* unmanaged<uint, float, void> SpriteAnimationSetSpeed;
    public delegate* unmanaged<uint, uint> NodeAddSpriteAnimation;

    // ===== Save/Load API =====
    public delegate* unmanaged<byte*, byte*, bool> SaveGameSave;
    public delegate* unmanaged<byte*, byte*> SaveGameLoad;
    public delegate* unmanaged<byte*, bool> SaveGameDelete;
    public delegate* unmanaged<byte*> SaveGameListSlots;

    // ===== Scene/Room Transition API =====
    public delegate* unmanaged<byte*, float, void> SceneSwitchScene;
    public delegate* unmanaged<byte*> SceneGetCurrentSceneName;
    public delegate* unmanaged<byte*, byte*, void> SceneSetTransitionData;
    public delegate* unmanaged<byte*, byte*> SceneGetTransitionData;
    public delegate* unmanaged<byte*, bool> SceneHasTransitionData;
    public delegate* unmanaged<void> SceneClearTransitionData;
    public delegate* unmanaged<byte*, void*, void> SceneRegisterScene;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct SRPPipelineDesc
{
    public uint VertexShader;
    public uint FragmentShader;
    public uint NumRenderTargets;
    public fixed uint RenderTargetFormats[8];
    public uint DepthStencilFormat;
    public uint SampleCount;
    public uint CullMode;
    public byte DepthTest;
    public byte DepthWrite;
    public byte DepthFunc;
    public byte BlendEnable;
    public byte BlendColorWriteMask;
    public fixed float ClearColor[4];
    public uint Topology;
    private fixed uint _padding[1];
}

/// <summary>
/// 采样器创建描述。内存布局与 C++ Prisma::Scripting::SRPSamplerDesc 一致。
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct SRPSamplerDesc
{
    public uint MinFilter;
    public uint MagFilter;
    public uint MipFilter;
    public uint AddressU;
    public uint AddressV;
    public uint AddressW;
}

internal static unsafe class Interop
{
    internal static PrismaAPI API;

    // 双缓冲区指针（每帧由 SyncBufferPointers 更新）
    internal static TransformDataLayout* TransformRead;
    internal static TransformDataLayout* TransformWrite;
    internal static RenderDataLayout* RenderData;

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
                      $"  并重新编译 Prisma.Bindings.dll 与 GameScripts.dll。";
            throw new InvalidOperationException(msg);
        }

        API = *api;
        // 初始指针基于当前 m_writeIndex(=0)：
        //   GetB = GetRead = &layoutB  (C# 从此读)
        //   GetA = GetWrite = &layoutA (C# 往此写)
        TransformRead = API.GetTransformB();
        TransformWrite = API.GetTransformA();
        RenderData = API.GetRenderData();
    }

    /// <summary>
    /// 每帧同步 C# 侧的双缓冲指针，使其与 C++ m_writeIndex 保持一致。
    /// C++ 是唯一的交换权威，每次 SwapBuffers 后 Read/Write 互换。
    /// </summary>
    internal static void SyncBufferPointers()
    {
        unsafe
        {
            TransformRead = API.GetTransformB();
            TransformWrite = API.GetTransformA();
        }
    }

    // ===== 字符串编组辅助 =====

    /// <summary>
    /// 将 C# string 转为以 null 结尾的 UTF-8 字节数组（配合 fixed 语句使用）
    /// </summary>
    internal static byte[] StringToUtf8(string s)
    {
        if (s == null) return new byte[] { 0 };
        return System.Text.Encoding.UTF8.GetBytes(s + "\0");
    }

    /// <summary>
    /// 将以 null 结尾的 UTF-8 指针转为 C# string
    /// </summary>
    internal static unsafe string Utf8ToString(byte* ptr)
    {
        if (ptr == null) return "";
        int len = 0;
        while (ptr[len] != 0) len++;
        return System.Text.Encoding.UTF8.GetString(ptr, len);
    }
}
