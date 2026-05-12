using System;
using System.Collections.Generic;

namespace Prisma.UI;

[Serializable]
public partial class Dropdown : UIComponent
{
    public List<string> Options { get; } = new();
    public int SelectedIndex { get; set; } = -1;
    public Text? CaptionText { get; set; }
    public Panel? PopupContainer { get; set; }
    public bool IsOpen { get; private set; }
    
    public event Action<int, string>? OnValueChanged;
    
    public string SelectedOption => SelectedIndex >= 0 && SelectedIndex < Options.Count 
        ? Options[SelectedIndex] 
        : "";
    
    public Dropdown()
    {
        ElementType = UIElementType.Dropdown;
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        
        if (CaptionText == null)
        {
            CaptionText = new Text { TextContent = "" };
        }
    }
    
    public void AddOption(string option)
    {
        Options.Add(option);
    }
    
    public void ClearOptions()
    {
        Options.Clear();
        SelectedIndex = -1;
    }
    
    public void Show()
    {
        IsOpen = true;
        
        if (PopupContainer != null)
        {
            PopupContainer.Visible = true;
        }
    }
    
    public void Hide()
    {
        IsOpen = false;
        
        if (PopupContainer != null)
        {
            PopupContainer.Visible = false;
        }
    }
    
    public void Select(int index)
    {
        if (index < 0 || index >= Options.Count) return;
        
        SelectedIndex = index;
        
        if (CaptionText != null)
        {
            CaptionText.TextContent = Options[index];
        }
        
        Hide();
        OnValueChanged?.Invoke(SelectedIndex, Options[index]);
    }
    
    public override void OnClick()
    {
        base.OnClick();
        
        if (IsOpen)
        {
            Hide();
        }
        else
        {
            Show();
        }
    }
}