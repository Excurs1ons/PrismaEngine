using System;

namespace PrismaEngine.UI;

[Serializable]
public struct ColorBlock
{
    public PrismaEngine.Color NormalColor;
    public PrismaEngine.Color HighlightedColor;
    public PrismaEngine.Color PressedColor;
    public PrismaEngine.Color SelectedColor;
    public PrismaEngine.Color DisabledColor;
    public float ColorMultiplier;
    
    public static ColorBlock Default => new()
    {
        NormalColor = PrismaEngine.Color.White,
        HighlightedColor = new PrismaEngine.Color(0.78f, 0.78f, 0.78f),
        PressedColor = new PrismaEngine.Color(0.55f, 0.55f, 0.55f),
        SelectedColor = new PrismaEngine.Color(0.68f, 0.68f, 0.68f),
        DisabledColor = new PrismaEngine.Color(0.5f, 0.5f, 0.5f, 0.5f),
        ColorMultiplier = 1f
    };
}

[Serializable]
public struct SpriteState
{
    public PrismaEngine.Color HighlightedSprite;
    public PrismaEngine.Color PressedSprite;
    public PrismaEngine.Color SelectedSprite;
    public PrismaEngine.Color DisabledSprite;
}