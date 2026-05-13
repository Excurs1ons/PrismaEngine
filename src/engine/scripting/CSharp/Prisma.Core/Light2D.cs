using System;

namespace Prisma;

public enum LightType
{
    Point = 0,
    Directional = 1,
    Spot = 2
}

public enum LightBlendMode
{
    Additive = 0,
    AlphaBlend = 1,
    Multiply = 2,
    Subtractive = 3
}

/**
 * @brief 2D 光源 C# 封装
 */
public class Light2D : IDisposable
{
    private uint _handle;
    private bool _disposed;

    public Light2D(LightType type = LightType.Point)
    {
        unsafe { _handle = NativeAPI.API.CreateLight((int)type); }
    }

    public uint Handle => _handle;

    public Vector2 Position
    {
        set { unsafe { NativeAPI.API.SetLightPos(_handle, value.X, value.Y); } }
    }

    public Vector3 Color
    {
        set { unsafe { NativeAPI.API.SetLightColor(_handle, value.X, value.Y, value.Z); } }
    }

    public float Intensity
    {
        set { unsafe { NativeAPI.API.SetLightIntensity(_handle, value); } }
    }

    public float Radius
    {
        set { unsafe { NativeAPI.API.SetLightRadius(_handle, value); } }
    }

    public float Falloff
    {
        set { unsafe { NativeAPI.API.SetLightFalloff(_handle, value); } }
    }

    public int Order
    {
        set { unsafe { NativeAPI.API.SetLightOrder(_handle, value); } }
    }

    public LightBlendMode BlendMode
    {
        set { unsafe { NativeAPI.API.SetLightBlendMode(_handle, (int)value); } }
    }

    /// <summary>
    /// 全局环境光颜色。（白光 (1,1,1) = 完全照亮，黑光 (0,0,0) = 只有光源区域）
    /// 可在场景初始化脚本中设置。
    /// </summary>
    public static Vector3 AmbientColor
    {
        set
        {
            unsafe { NativeAPI.API.SetAmbientLight(value.X, value.Y, value.Z); }
        }
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            if (_handle != 0)
            {
                unsafe { NativeAPI.API.DestroyLight(_handle); }
                _handle = 0;
            }
            _disposed = true;
            GC.SuppressFinalize(this);
        }
    }

    ~Light2D() => Dispose();
}
