using System;

namespace Prisma.UI;

[Serializable]
public struct ColorBlock
{
    public Prisma.Color NormalColor;
    public Prisma.Color HighlightedColor;
    public Prisma.Color PressedColor;
    public Prisma.Color SelectedColor;
    public Prisma.Color DisabledColor;
    public float ColorMultiplier;
    
    public static ColorBlock Default => new()
    {
        NormalColor = Prisma.Color.White,
        HighlightedColor = new Prisma.Color(0.78f, 0.78f, 0.78f),
        PressedColor = new Prisma.Color(0.55f, 0.55f, 0.55f),
        SelectedColor = new Prisma.Color(0.68f, 0.68f, 0.68f),
        DisabledColor = new Prisma.Color(0.5f, 0.5f, 0.5f, 0.5f),
        ColorMultiplier = 1f
    };
}

[Serializable]
public struct SpriteState
{
    public Prisma.Color HighlightedSprite;
    public Prisma.Color PressedSprite;
    public Prisma.Color SelectedSprite;
    public Prisma.Color DisabledSprite;
}