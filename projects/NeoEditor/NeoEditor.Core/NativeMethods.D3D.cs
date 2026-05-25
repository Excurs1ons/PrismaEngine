using System.Runtime.InteropServices;

namespace NeoEditor.Core.Interop;

// ============================================================================
// D3D11 Driver Type
// ============================================================================
internal enum D3D_DRIVER_TYPE : uint
{
    Unknown = 0,
    Hardware = 1,
    Reference = 2,
    Null = 3,
    Software = 4,
    Warp = 5,
}

// ============================================================================
// D3D11 Feature Level
// ============================================================================
internal enum D3D_FEATURE_LEVEL : uint
{
    _9_1  = 0x9100,
    _9_2  = 0x9200,
    _9_3  = 0x9300,
    _10_0 = 0xA000,
    _10_1 = 0xA100,
    _11_0 = 0xB000,
    _11_1 = 0xB100,
}

// ============================================================================
// D3D11 Create Device Flags
// ============================================================================
[Flags]
internal enum D3D11_CREATE_DEVICE_FLAG : uint
{
    None                                           = 0,
    SingleThreaded                                 = 0x1,
    Debug                                          = 0x2,
    SwitchToRef                                    = 0x4,
    PreventInternalThreadingOptimizations          = 0x8,
    BgraSupport                                    = 0x20,
    VideoSupport                                   = 0x800,
}

// ============================================================================
// DXGI Format
// ============================================================================
internal enum DXGI_FORMAT : uint
{
    Unknown                      = 0,
    R32G32B32A32_Typeless        = 1,
    R32G32B32A32_Float           = 2,
    R32G32B32A32_UInt            = 3,
    R32G32B32A32_SInt            = 4,
    R32G32B32_Typeless           = 5,
    R32G32B32_Float              = 6,
    R32G32B32_UInt               = 7,
    R32G32B32_SInt               = 8,
    R16G16B16A16_Typeless        = 9,
    R16G16B16A16_Float           = 10,
    R16G16B16A16_UNorm           = 11,
    R16G16B16A16_UInt            = 12,
    R16G16B16A16_SNorm           = 13,
    R16G16B16A16_SInt            = 14,
    R32G32_Typeless              = 15,
    R32G32_Float                 = 16,
    R32G32_UInt                  = 17,
    R32G32_SInt                  = 18,
    R32G8X24_Typeless            = 19,
    D32_Float_S8X24_UInt         = 20,
    R32_Float_X8X24_Typeless     = 21,
    X32_Typeless_G8X24_UInt      = 22,
    R10G10B10A2_Typeless         = 23,
    R10G10B10A2_UNorm            = 24,
    R10G10B10A2_UInt             = 25,
    R11G11B10_Float              = 26,
    R8G8B8A8_Typeless            = 27,
    R8G8B8A8_UNorm               = 28,
    R8G8B8A8_UNorm_SRgb          = 29,
    R8G8B8A8_UInt                = 30,
    R8G8B8A8_SNorm               = 31,
    R8G8B8A8_SInt                = 32,
    R16G16_Typeless              = 33,
    R16G16_Float                 = 34,
    R16G16_UNorm                 = 35,
    R16G16_UInt                  = 36,
    R16G16_SNorm                 = 37,
    R16G16_SInt                  = 38,
    R32_Typeless                 = 39,
    D32_Float                    = 40,
    R32_Float                    = 41,
    R32_UInt                     = 42,
    R32_SInt                     = 43,
    R24G8_Typeless               = 44,
    D24_UNorm_S8_UInt            = 45,
    R24_UNorm_X8_Typeless        = 46,
    X24_Typeless_G8_UInt         = 47,
    R8G8_Typeless                = 48,
    R8G8_UNorm                   = 49,
    R8G8_UInt                    = 50,
    R8G8_SNorm                   = 51,
    R8G8_SInt                    = 52,
    R16_Typeless                 = 53,
    R16_Float                    = 54,
    D16_UNorm                    = 55,
    R16_UNorm                    = 56,
    R16_UInt                     = 57,
    R16_SNorm                    = 58,
    R16_SInt                     = 59,
    R8_Typeless                  = 60,
    R8_UNorm                     = 61,
    R8_UInt                      = 62,
    R8_SNorm                     = 63,
    R8_SInt                      = 64,
    A8_UNorm                     = 65,
    R1_UNorm                     = 66,
    R9G9B9E5_SharedExp           = 67,
    R8G8_B8G8_UNorm              = 68,
    G8R8_G8B8_UNorm              = 69,
    BC1_Typeless                 = 70,
    BC1_UNorm                    = 71,
    BC1_UNorm_SRgb               = 72,
    BC2_Typeless                 = 73,
    BC2_UNorm                    = 74,
    BC2_UNorm_SRgb               = 75,
    BC3_Typeless                 = 76,
    BC3_UNorm                    = 77,
    BC3_UNorm_SRgb               = 78,
    BC4_Typeless                 = 79,
    BC4_UNorm                    = 80,
    BC4_SNorm                    = 81,
    BC5_Typeless                 = 82,
    BC5_UNorm                    = 83,
    BC5_SNorm                    = 84,
    B5G6R5_UNorm                 = 85,
    B5G5R5A1_UNorm               = 86,
    B8G8R8A8_UNorm               = 87,
    B8G8R8X8_UNorm               = 88,
    R10G10B10A2_Typeless_Remap   = 89,
    R10G10B10A2_UNorm_Remap      = 90,
    R10G10B10A2_UInt_Remap       = 91,
    B8G8R8A8_Typeless            = 92,
    B8G8R8A8_UNorm_SRgb          = 93,
    B8G8R8X8_Typeless            = 94,
    B8G8R8X8_UNorm_SRgb          = 95,
    BC6H_Typeless                = 96,
    BC6H_Uf16                    = 97,
    BC6H_Sf16                    = 98,
    BC7_Typeless                 = 99,
    BC7_UNorm                    = 100,
    BC7_UNorm_SRgb               = 101,
}

// ============================================================================
// DXGI Swap Effect
// ============================================================================
internal enum DXGI_SWAP_EFFECT : uint
{
    Discard           = 0,
    Sequential        = 1,
    FlipSequential    = 3,
    FlipDiscard       = 4,
}

// ============================================================================
// DXGI Scaling
// ============================================================================
internal enum DXGI_SCALING : uint
{
    Stretch   = 0,
    None      = 1,
    Aspect    = 2,
}

// ============================================================================
// DXGI Alpha Mode
// ============================================================================
internal enum DXGI_ALPHA_MODE : uint
{
    Unspecified    = 0,
    Premultiplied  = 1,
    Straight       = 2,
    Ignore         = 3,
}

// ============================================================================
// DXGI Swap Chain Flags
// ============================================================================
[Flags]
internal enum DXGI_SWAP_CHAIN_FLAG : uint
{
    None                        = 0,
    Nonprerotated               = 1,
    AllowModeSwitch             = 2,
    GdiCompatible               = 4,
    RestrictedContent           = 8,
    RestrictSharedResourceDriver = 16,
    DisplayOnly                 = 32,
    ForegroundLayer             = 64,
    FullscreenVideo             = 128,
    YuvVideo                    = 256,
    HwProtected                 = 512,
    AllowTearing                = 1024,
    RestrictedToCurrentHwDevice = 2048,
}

// ============================================================================
// DXGI Adapter Flags
// ============================================================================
[Flags]
internal enum DXGI_ADAPTER_FLAG : uint
{
    None           = 0,
    Remote         = 1,
    Software       = 2,
    Academy        = 4,
    NvidiaPhysX    = 8,
}

// ============================================================================
// DXGI Usage
// ============================================================================
internal static class DXGI_USAGE
{
    public const uint ShaderInput       = 0x00000010;
    public const uint RenderTargetOutput = 0x00000020;
    public const uint BackBuffer        = 0x00000040;
    public const uint Shared            = 0x00000080;
    public const uint ReadOnly          = 0x00000100;
    public const uint DiscardOnPresent  = 0x00000200;
    public const uint UnorderedAccess   = 0x00000400;
}

// ============================================================================
// DXGI Sample Desc
// ============================================================================
[StructLayout(LayoutKind.Sequential)]
internal struct DXGI_SAMPLE_DESC
{
    public uint Count;
    public uint Quality;
}

// ============================================================================
// DXGI Swap Chain Desc1
// ============================================================================
[StructLayout(LayoutKind.Sequential)]
internal struct DXGI_SWAP_CHAIN_DESC1
{
    public uint             Width;
    public uint             Height;
    public DXGI_FORMAT      Format;
    public int              Stereo;       // BOOL
    public DXGI_SAMPLE_DESC SampleDesc;
    public uint             BufferUsage;
    public uint             BufferCount;
    public DXGI_SCALING     Scaling;
    public DXGI_SWAP_EFFECT SwapEffect;
    public DXGI_ALPHA_MODE  AlphaMode;
    public uint             Flags;
}

// ============================================================================
// LUID (Locally Unique Identifier)
// Matching Vulkan's VkPhysicalDeviceIDProperties::deviceLUID (8 bytes)
// ============================================================================
[StructLayout(LayoutKind.Sequential)]
internal struct LUID
{
    public uint LowPart;
    public int  HighPart;

    /// Convert to a single 64-bit value for comparison with Vulkan LUID.
    public readonly long ToInt64() => (long)HighPart << 32 | LowPart;
}

// ============================================================================
// DXGI Adapter Desc1
// ============================================================================
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct DXGI_ADAPTER_DESC1
{
    /// WCHAR Description[128]
    public const int DescriptionLength = 128;

    public fixed char Description[DescriptionLength];
    public uint       VendorId;
    public uint       DeviceId;
    public uint       SubSysId;
    public uint       Revision;
    public UIntPtr    DedicatedVideoMemory;
    public UIntPtr    DedicatedSystemMemory;
    public UIntPtr    SharedSystemMemory;
    public LUID       AdapterLuid;
    public uint       Flags;
}

// ============================================================================
// GUID Constants for COM Interfaces
// ============================================================================
internal static class DXGIGuid
{
    // IID_IDXGIDevice        = 54ec77fa-1377-44e6-8c32-88fd5f44c84c
    public static readonly Guid IDXGIDevice = new("54ec77fa-1377-44e6-8c32-88fd5f44c84c");

    // IID_IDXGIDeviceSubObject = 3d3e0379-f9de-4d58-bb6c-18d62992f1a6
    public static readonly Guid IDXGIDeviceSubObject = new("3d3e0379-f9de-4d58-bb6c-18d62992f1a6");

    // IID_IDXGIAdapter       = 2411e7e1-12ac-4ccf-bd14-9798e8534dc0
    public static readonly Guid IDXGIAdapter = new("2411e7e1-12ac-4ccf-bd14-9798e8534dc0");

    // IID_IDXGIAdapter1      = 29038f03-3839-4ffe-9412-0b2e2fec6c5f
    public static readonly Guid IDXGIAdapter1 = new("29038f03-3839-4ffe-9412-0b2e2fec6c5f");

    // IID_IDXGIFactory       = 7b7166ec-21c7-44ae-b21a-c9ae321ae369
    public static readonly Guid IDXGIFactory = new("7b7166ec-21c7-44ae-b21a-c9ae321ae369");

    // IID_IDXGIFactory1      = 770aae78-f26f-4dba-a829-253c83d1b387
    public static readonly Guid IDXGIFactory1 = new("770aae78-f26f-4dba-a829-253c83d1b387");

    // IID_IDXGIFactory2      = 50c83a1c-e072-4c48-87b0-3630fa36a6d0
    public static readonly Guid IDXGIFactory2 = new("50c83a1c-e072-4c48-87b0-3630fa36a6d0");

    // IID_IDXGISwapChain     = 310eb36a-d2e8-4180-9f46-1318a8c6e283
    public static readonly Guid IDXGISwapChain = new("310eb36a-d2e8-4180-9f46-1318a8c6e283");

    // IID_IDXGISwapChain1    = 4a8790b9-8792-427c-bac6-b2b0a6e3c7d3
    public static readonly Guid IDXGISwapChain1 = new("4a8790b9-8792-427c-bac6-b2b0a6e3c7d3");

    // IID_ISwapChainPanelNative = 3e3f1d7d-e21f-43e1-bc1c-fad9306572d7
    public static readonly Guid ISwapChainPanelNative = new("3E3F1D7D-E21F-43E1-BC1C-FAD9306572D7");
}

internal static class D3D11Guid
{
    // IID_ID3D11Device       = db6f6ddb-ac77-4e88-8253-819df9bbf140
    public static readonly Guid ID3D11Device = new("db6f6ddb-ac77-4e88-8253-819df9bbf140");

    // IID_ID3D11Device1      = a04bfb29-08ef-43d6-a49c-a9bdbdcbe686
    public static readonly Guid ID3D11Device1 = new("a04bfb29-08ef-43d6-a49c-a9bdbdcbe686");

    // IID_ID3D11DeviceContext = c0bfa96c-e089-44fb-8eaf-26f87994ae84
    public static readonly Guid ID3D11DeviceContext = new("c0bfa96c-e089-44fb-8eaf-26f87994ae84");

    // IID_ID3D11Texture2D    = 6f15aaf2-d208-4e89-9ab4-489535d34f9c
    public static readonly Guid ID3D11Texture2D = new("6f15aaf2-d208-4e89-9ab4-489535d34f9c");
}

// ============================================================================
// D3D11 SDK Version
// ============================================================================
internal static class D3D11Constants
{
    /// D3D11_SDK_VERSION = 7 (unchanged since Windows 7)
    public const uint SDKVersion = 7;
}

// ============================================================================
// P/Invoke Declarations for d3d11.dll and dxgi.dll
// ============================================================================
internal static class NativeMethodsD3D
{
    // --------------------------------------------------------------------
    // D3D11CreateDevice
    // --------------------------------------------------------------------
    /// Creates a Direct3D 11 device.
    [DllImport("d3d11.dll", CallingConvention = CallingConvention.StdCall)]
    internal static extern int D3D11CreateDevice(
        IntPtr              pAdapter,          // IDXGIAdapter* or null
        D3D_DRIVER_TYPE     DriverType,        // Must be Unknown if pAdapter != null
        IntPtr              Software,          // HMODULE for software rasterizer
        uint                Flags,
        [In] ref D3D_FEATURE_LEVEL pFeatureLevels,
        uint                FeatureLevels,
        uint                SDKVersion,        // D3D11_SDK_VERSION = 7
        out IntPtr          ppDevice,          // ID3D11Device*
        out D3D_FEATURE_LEVEL pFeatureLevel,
        out IntPtr          ppImmediateContext); // ID3D11DeviceContext*

    // --------------------------------------------------------------------
    // CreateDXGIFactory1
    // --------------------------------------------------------------------
    [DllImport("dxgi.dll", CallingConvention = CallingConvention.StdCall)]
    internal static extern int CreateDXGIFactory1(
        [In] ref Guid riid,
        out IntPtr   ppFactory);

    // --------------------------------------------------------------------
    // CreateDXGIFactory2
    // --------------------------------------------------------------------
    [DllImport("dxgi.dll", CallingConvention = CallingConvention.StdCall)]
    internal static extern int CreateDXGIFactory2(
        uint        Flags,      // DXGI_CREATE_FACTORY_DEBUG = 0x01
        [In] ref Guid riid,
        out IntPtr  ppFactory);
}
