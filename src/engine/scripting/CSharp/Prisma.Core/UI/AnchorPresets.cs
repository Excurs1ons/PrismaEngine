using System;

namespace Prisma.UI;

[Flags]
public enum AnchorPresets
{
    None = 0,
    TopLeft = 1 << 0,
    TopCenter = 1 << 1,
    TopRight = 1 << 2,
    MiddleLeft = 1 << 3,
    MiddleCenter = 1 << 4,
    MiddleRight = 1 << 5,
    BottomLeft = 1 << 6,
    BottomCenter = 1 << 7,
    BottomRight = 1 << 8,
    StretchHorizontal = 1 << 9,
    StretchVertical = 1 << 10,
    StretchAll = StretchHorizontal | StretchVertical,
    Custom = 1 << 11
}

public enum TextAlignment
{
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
}

public enum ImageType
{
    Simple,
    Sliced,
    Tiled,
    Filled
}