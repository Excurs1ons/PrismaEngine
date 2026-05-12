using System;

namespace Prisma.UI;

[Serializable]
public partial class Panel : UIComponent
{
    public Image? Background { get; set; }
    public bool ClipChildren { get; set; } = false;
    public bool UseLayout { get; set; } = false;
    
    public Panel()
    {
        ElementType = UIElementType.Panel;
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        
        if (Background == null)
        {
            Background = new Image { Color = Color.White };
        }
    }
}