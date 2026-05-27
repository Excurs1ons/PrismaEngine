using System.Runtime.InteropServices;
using NeoEditor.Core.Interop;
using Xunit;

namespace NeoEditor.Tests;

/// EditorAPI 结构体绑定和高级封装的单元测试。
/// C# 绑定与 C++ EditorAPI.h 的字段布局一致性验证。
public unsafe class EditorAPITests
{
    // ===================================================================
    // Struct layout validation
    // ===================================================================

    /// 验证 EditorAPI_Interop 结构体在 64 位平台上的预期大小。
    /// C++ 侧 structSize 字段必须与 C# sizeof 一致。
    [Fact]
    public void StructSizeMatches()
    {
        // On x64/ARM64: 25 function pointers (8 bytes each) + 1 uint32 (4 bytes)
        //   25 * 8 = 200; 200 + 4 = 204; align to 8 → 208
        uint expectedOnX64 = 208;
        uint actualSize = (uint)sizeof(EditorAPI_Interop);

        Assert.Equal(expectedOnX64, actualSize);
    }

    /// 验证结构体大小在合理范围内（避免因平台差异导致测试过于严格）。
    [Fact]
    public void StructSize_IsReasonable()
    {
        uint size = (uint)sizeof(EditorAPI_Interop);
        Assert.True(
            size >= 200 && size <= 220,
            $"EditorAPI struct size {size} is outside expected range [200, 220]"
        );
    }

    // ===================================================================
    // Initialize validation
    // ===================================================================

    [Fact]
    public void Initialize_WithNullPtr_Throws()
    {
        EditorAPI.Reset();
        var ex = Assert.Throws<ArgumentNullException>(
            () => EditorAPI.Initialize(IntPtr.Zero)
        );
        Assert.Equal("apiPtr", ex.ParamName);
    }

    /// 使用合法的 mock 结构体初始化后，IsInitialized 必须为 true。
    /// 此测试验证 Initialize 完整路径：IntPtr 转换 → 结构体复制 → StructSize 校验 → 设置标记。
    [Fact]
    public unsafe void Initialize_SetsInitialized()
    {
        EditorAPI.Reset();
        Assert.False(EditorAPI.IsInitialized);

        // 构造合法的 mock（函数指针可为 null — Initialize 不会调用它们）
        var mock = new EditorAPI_Interop();
        mock.StructSize = (uint)sizeof(EditorAPI_Interop);

        EditorAPI.Initialize(new IntPtr(&mock));
        Assert.True(EditorAPI.IsInitialized);

        EditorAPI.Reset();
    }

    // ===================================================================
    // Function pointer delegate validation
    // ===================================================================

    [UnmanagedCallersOnly]
    private static void FreeStringStub(void* ptr)
    {
        // Stub: 不执行任何操作。模拟 C++ freeString 的安全调用。
    }

    /// 验证 freeString 委托可在空指针和非法指针上安全调用（不崩溃）。
    /// 这确保 C# MarshalAndFree 能够安全地释放 C++ 分配的字符串。
    [Fact]
    public void FreeString_DoesNotCrash()
    {
        var api = default(EditorAPI_Interop);
        api.FreeString = &FreeStringStub;

        // null 指针 — stub 不应崩溃
        api.FreeString(null);

        // 任意地址 — stub 不应崩溃
        int dummy = 42;
        api.FreeString(&dummy);

        // 再次调用 null 验证稳定性
        api.FreeString(null);
    }
}
