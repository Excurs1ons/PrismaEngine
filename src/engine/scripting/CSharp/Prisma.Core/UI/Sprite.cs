namespace Prisma.UI;

public class Sprite
{
    public uint Id { get; }
    public string Name { get; }
    public Prisma.Rect Rect { get; }
    public Prisma.Vector2 Pivot { get; }
    public Prisma.Vector2 Size { get; }
    
    internal Sprite(uint id, string name, Prisma.Rect rect, Prisma.Vector2 pivot)
    {
        Id = id;
        Name = name;
        Rect = rect;
        Pivot = pivot;
        Size = new Prisma.Vector2(rect.Width, rect.Height);
    }
}