namespace PrismaEngine.UI;

public class Sprite
{
    public uint Id { get; }
    public string Name { get; }
    public PrismaEngine.Rect Rect { get; }
    public PrismaEngine.Vector2 Pivot { get; }
    public PrismaEngine.Vector2 Size { get; }
    
    internal Sprite(uint id, string name, PrismaEngine.Rect rect, PrismaEngine.Vector2 pivot)
    {
        Id = id;
        Name = name;
        Rect = rect;
        Pivot = pivot;
        Size = new PrismaEngine.Vector2(rect.Width, rect.Height);
    }
}