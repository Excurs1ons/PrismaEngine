using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Prisma;

/// <summary>
/// 非泛型接口，允许 World 在不知道 T 的情况下遍历所有池。
/// </summary>
internal interface IComponentPool
{
    void ClearSlot(uint entityHandle);
    void Dispose();
}

/// <summary>
/// 非托管组件池。按 entity 索引 (handle & 0xFFFF) 连续存储 T，位标记跟踪存在性。
/// - Get() = 直接指针运算，无分支，零开销
/// - Has() = 位掩码检查
/// - 自动扩容，非托管内存，无 GC 压力
/// </summary>
public sealed unsafe class ComponentPool<T> : IComponentPool, IDisposable where T : unmanaged
{
    private T* _data;
    private ulong* _mask;
    private int _capacity;

    public ComponentPool(int initialCapacity = 1024)
    {
        _capacity = initialCapacity;
        _data = (T*)NativeMemory.AllocZeroed((nuint)initialCapacity * (nuint)sizeof(T));
        _mask = (ulong*)NativeMemory.AllocZeroed((nuint)((initialCapacity + 63) / 64 * sizeof(ulong)));
    }

    /// <summary>获取 entity 的组件引用。不检查存在性——由调用者保证。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public ref T Get(uint entityHandle)
    {
        uint idx = entityHandle & 0xFFFF;
        if (idx >= _capacity) Grow((int)idx + 1);
        return ref _data[idx];
    }

    /// <summary>安全的"存在则获取"模式。返回指针，null 表示不存在。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public T* TryGet(uint entityHandle)
    {
        uint idx = entityHandle & 0xFFFF;
        if (idx < _capacity && (_mask[idx >> 6] & (1UL << (int)(idx & 0x3F))) != 0)
            return &_data[idx];
        return null;
    }

    /// <summary>检查 entity 是否持有该组件。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool Has(uint entityHandle)
    {
        uint idx = entityHandle & 0xFFFF;
        return idx < _capacity && (_mask[idx >> 6] & (1UL << (int)(idx & 0x3F))) != 0;
    }

    /// <summary>标记 entity 持有该组件（不清零 Get 返回的数据）。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void Set(uint entityHandle)
    {
        uint idx = entityHandle & 0xFFFF;
        if (idx >= _capacity) Grow((int)idx + 1);
        _mask[idx >> 6] |= 1UL << (int)(idx & 0x3F);
    }

    /// <summary>移除标记并清零数据。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void Remove(uint entityHandle)
    {
        uint idx = entityHandle & 0xFFFF;
        if (idx < _capacity)
        {
            _data[idx] = default;
            _mask[idx >> 6] &= ~(1UL << (int)(idx & 0x3F));
        }
    }

    /// <summary>entity 销毁时清理标记（不写内存，避免无意义清零）。</summary>
    void IComponentPool.ClearSlot(uint entityHandle)
    {
        uint idx = entityHandle & 0xFFFF;
        if (idx < _capacity)
            _mask[idx >> 6] &= ~(1UL << (int)(idx & 0x3F));
    }

    /// <summary>获取当前容量。</summary>
    public int Capacity => _capacity;

    /// <summary>获取当前标记的组件数。</summary>
    public int Count
    {
        get
        {
            int count = 0;
            int words = (_capacity + 63) / 64;
            for (int i = 0; i < words; i++)
                count += System.Numerics.BitOperations.PopCount(_mask[i]);
            return count;
        }
    }

    private void Grow(int minCapacity)
    {
        int newCap = Math.Max(_capacity * 2, minCapacity);
        nuint oldBytes = (nuint)_capacity * (nuint)sizeof(T);
        nuint newBytes = (nuint)newCap * (nuint)sizeof(T);

        var newData = (T*)NativeMemory.AllocZeroed(newBytes);
        if (_data != null)
        {
            NativeMemory.Copy(_data, newData, oldBytes);
            NativeMemory.Free(_data);
        }
        _data = newData;

        nuint oldMaskWords = (nuint)(_capacity + 63) / 64;
        nuint newMaskWords = (nuint)(newCap + 63) / 64;
        var newMask = (ulong*)NativeMemory.AllocZeroed(newMaskWords * (nuint)sizeof(ulong));
        if (_mask != null)
        {
            NativeMemory.Copy(_mask, newMask, oldMaskWords * (nuint)sizeof(ulong));
            NativeMemory.Free(_mask);
        }
        _mask = newMask;

        _capacity = newCap;
    }

    public void Dispose()
    {
        if (_data != null) { NativeMemory.Free(_data); _data = null; }
        if (_mask != null) { NativeMemory.Free(_mask); _mask = null; }
        _capacity = 0;
        GC.SuppressFinalize(this);
    }

    ~ComponentPool() { Dispose(); }
}
