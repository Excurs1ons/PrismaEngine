using System;
using System.Text;

namespace PrismaEngine.UI;

[Serializable]
public partial class InputField : UIComponent
{
    public string Text { get; set; } = "";
    public string Placeholder { get; set; } = "";
    public int CharacterLimit { get; set; } = 0;
    public bool IsPassword { get; set; } = false;
    public char PasswordChar { get; set; } = '*';
    
    public Text? ContentText { get; set; }
    public Text? PlaceholderText { get; set; }
    public Image? Background { get; set; }
    
    public event Action<string>? OnTextChanged;
    public event Action<string>? OnEndEdit;
    
    private StringBuilder _inputBuffer = new();
    
    public InputField()
    {
        ElementType = UIElementType.TextField;
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        
        if (ContentText == null)
        {
            ContentText = new Text { TextContent = "" };
        }
        
        if (PlaceholderText == null)
        {
            PlaceholderText = new Text { TextContent = Placeholder };
        }
    }
    
    public void SetText(string text)
    {
        if (CharacterLimit > 0 && text.Length > CharacterLimit)
        {
            text = text.Substring(0, CharacterLimit);
        }
        
        Text = text;
        UpdateDisplay();
        OnTextChanged?.Invoke(Text);
    }
    
    public void AppendCharacter(char c)
    {
        if (CharacterLimit > 0 && Text.Length >= CharacterLimit)
        {
            return;
        }
        
        _inputBuffer.Append(c);
        SetText(_inputBuffer.ToString());
    }
    
    public void DeleteLastCharacter()
    {
        if (_inputBuffer.Length > 0)
        {
            _inputBuffer.Length--;
            SetText(_inputBuffer.ToString());
        }
    }
    
    public void Clear()
    {
        _inputBuffer.Clear();
        Text = "";
        UpdateDisplay();
    }
    
    private void UpdateDisplay()
    {
        string displayText = IsPassword 
            ? new string(PasswordChar, Text.Length) 
            : Text;
        
        if (ContentText != null)
        {
            ContentText.TextContent = displayText;
        }
        
        if (PlaceholderText != null)
        {
            PlaceholderText.Visible = string.IsNullOrEmpty(Text);
        }
    }
    
    public void Submit()
    {
        OnEndEdit?.Invoke(Text);
    }
}