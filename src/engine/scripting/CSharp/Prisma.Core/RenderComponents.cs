namespace Prisma;

/// <summary>
/// 可渲染组件的标记接口。用于类型层级，无法用于 ComponentPool（需要 unmanaged）。
/// </summary>
public interface IRenderComponent { }

/// <summary>
/// 精灵渲染组件。配合 Transform（C++ SoA）使用。
/// 存在 ComponentPool 中供 C# 游戏逻辑判断渲染类型。
/// </summary>
public struct SpriteRendererComponent : IRenderComponent
{
    public uint TextureHandle;
    public float SortingOrder;
}

/// <summary>
/// 网格渲染组件。
/// </summary>
public struct MeshRendererComponent : IRenderComponent
{
    public uint MeshHandle;
    public uint MaterialHandle;
    public bool CastShadows;
}
