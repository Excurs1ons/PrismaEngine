using System.Collections.Concurrent;
using System.Diagnostics;
using Microsoft.UI.Input;
using Windows.System;
using Windows.UI.Core;

namespace NeoEditor.Core;

/// 输入事件类型，用于区分不同类型的输入事件。
internal enum InputEventType
{
    KeyDown,
    KeyUp,
    MouseMove,
    MouseDown,
    MouseUp,
    MouseWheel
}

/// 输入事件结构体，包含所有的输入事件数据。
/// 通过消息队列投递到引擎线程，由引擎线程消费。
internal struct InputEvent
{
    public InputEventType Type;
    public int Key;           // KeyDown/Up: 引擎 KeyCode 值 (SDL scancode)
    public int MouseX, MouseY;         // MouseMove/Pressed: 鼠标位置
    public int DeltaX, DeltaY;         // MouseMove: 鼠标增量
    public int WheelDelta;             // MouseWheel: 滚轮增量
    public bool IsDown;                // MouseDown/Up: 按下/释放
}

/// 输入路由 (T14): WinUI3 键盘/鼠标事件 → Engine 后台线程的输入路由。
/// 将 WinUI3 输入事件通过消息队列投递到引擎线程。
///
/// 设计:
/// - InputRouter 运行在 UI 线程，接收 WinUI3 原生输入事件
/// - 将事件转换为 InputEvent 结构体，通过 Action 闭包投递到 UIToEngineQueue
/// - 引擎线程在每帧 Drain 时执行这些 Action，调用 InputManager API
/// - 编辑器热键（F/Delete/Ctrl+S）直接在 UI 线程通过 EditorAPI 处理
internal sealed class InputRouter : IDisposable
{
    private readonly UIToEngineQueue _inputQueue;
    private readonly HashSet<VirtualKey> _pressedKeys = new();
    private int _mouseX, _mouseY;
    private bool _isRightButtonDown;
    private bool _disposed;

    public InputRouter(UIToEngineQueue inputQueue)
    {
        _inputQueue = inputQueue ?? throw new ArgumentNullException(nameof(inputQueue));
    }

    /// 当前鼠标 X 坐标（UI 线程坐标）。
    public int MouseX => _mouseX;

    /// 当前鼠标 Y 坐标（UI 线程坐标）。
    public int MouseY => _mouseY;

    /// 鼠标右键是否处于按下状态。
    public bool IsRightButtonDown => _isRightButtonDown;

    // ================================================================
    // 键盘事件
    // ================================================================

    /// 处理 WinUI3 KeyDown 事件。
    /// 先检查是否为编辑器热键（F/Delete/Ctrl+S），如果是则直接处理不投递。
    /// 否则将按键事件投递到引擎线程。
    public void OnKeyDown(VirtualKey key)
    {
        if (_disposed) return;

        // 修饰键（Shift/Ctrl/Alt）可能重复触发 KeyDown，去重
        if (IsModifierKey(key))
        {
            if (_pressedKeys.Contains(key))
                return;
        }
        else if (!_pressedKeys.Add(key))
        {
            return; // 已按下，忽略重复
        }

        // 检查编辑器热键（优先处理，不投递到引擎）
        if (HandleEditorHotkey(key))
            return;

        // 投递到引擎线程
        int engineKey = MapVirtualKey(key);
        if (engineKey != 0)
        {
            EnqueueInputEvent(new InputEvent
            {
                Type = InputEventType.KeyDown,
                Key = engineKey
            });
        }
    }

    /// 处理 WinUI3 KeyUp 事件。
    /// 从按下状态集合移除，并投递到引擎线程。
    public void OnKeyUp(VirtualKey key)
    {
        if (_disposed) return;

        if (!_pressedKeys.Remove(key))
            return; // 未记录按下，忽略

        int engineKey = MapVirtualKey(key);
        if (engineKey != 0)
        {
            EnqueueInputEvent(new InputEvent
            {
                Type = InputEventType.KeyUp,
                Key = engineKey
            });
        }
    }

    /// 焦点丢失时释放所有按键。
    /// 遍历所有按下的键，逐个发送 KeyUp 事件并清空状态。
    public void ReleaseAllKeys()
    {
        if (_disposed) return;

        foreach (var key in _pressedKeys)
        {
            int engineKey = MapVirtualKey(key);
            if (engineKey != 0)
            {
                EnqueueInputEvent(new InputEvent
                {
                    Type = InputEventType.KeyUp,
                    Key = engineKey
                });
            }
        }

        _pressedKeys.Clear();
        _isRightButtonDown = false;
    }

    // ================================================================
    // 鼠标事件
    // ================================================================

    /// 处理 WinUI3 PointerMoved 事件。
    /// 计算鼠标增量并投递到引擎线程。
    public void OnPointerMoved(int x, int y)
    {
        if (_disposed) return;

        int deltaX = x - _mouseX;
        int deltaY = y - _mouseY;
        _mouseX = x;
        _mouseY = y;

        EnqueueInputEvent(new InputEvent
        {
            Type = InputEventType.MouseMove,
            MouseX = x,
            MouseY = y,
            DeltaX = deltaX,
            DeltaY = deltaY
        });
    }

    /// 处理 WinUI3 PointerPressed 事件。
    /// 检查鼠标按钮状态并投递 MouseDown 事件。
    public void OnPointerPressed(PointerPoint point)
    {
        if (_disposed) return;

        var props = point.Properties;

        if (props.IsRightButtonPressed)
        {
            _isRightButtonDown = true;
            EnqueueInputEvent(new InputEvent
            {
                Type = InputEventType.MouseDown,
                MouseX = (int)point.Position.X,
                MouseY = (int)point.Position.Y,
                IsDown = true
            });
        }
    }

    /// 处理 WinUI3 PointerReleased 事件。
    /// 检查鼠标按钮状态并投递 MouseUp 事件。
    public void OnPointerReleased(PointerPoint point)
    {
        if (_disposed) return;

        var props = point.Properties;

        // 只有当按钮从按下变为释放时才发送事件
        if (!props.IsRightButtonPressed && _isRightButtonDown)
        {
            _isRightButtonDown = false;
            EnqueueInputEvent(new InputEvent
            {
                Type = InputEventType.MouseUp,
                MouseX = (int)point.Position.X,
                MouseY = (int)point.Position.Y,
                IsDown = false
            });
        }
    }

    /// 处理 WinUI3 PointerWheelChanged 事件。
    /// 投递滚轮事件到引擎线程（用于摄像机缩放）。
    public void OnPointerWheelChanged(int delta)
    {
        if (_disposed) return;

        EnqueueInputEvent(new InputEvent
        {
            Type = InputEventType.MouseWheel,
            WheelDelta = delta
        });
    }

    // ================================================================
    // 编辑器热键
    // ================================================================

    /// 处理编辑器热键。
    /// 返回 true 表示已处理（不再投递到引擎线程）。
    private bool HandleEditorHotkey(VirtualKey key)
    {
        bool ctrl = _pressedKeys.Contains(VirtualKey.Control);

        // Ctrl+S: 保存场景
        if (ctrl && key == VirtualKey.S)
        {
            Debug.WriteLine("[InputRouter] Hotkey: Ctrl+S — Save scene");
            EditorAPI.ExecuteCommand("save scene");
            return true;
        }

        // Ctrl+Z: 撤销
        if (ctrl && key == VirtualKey.Z)
        {
            Debug.WriteLine("[InputRouter] Hotkey: Ctrl+Z — Undo");
            EditorAPI.ExecuteCommand("undo");
            return true;
        }

        // Ctrl+Y: 重做
        if (ctrl && key == VirtualKey.Y)
        {
            Debug.WriteLine("[InputRouter] Hotkey: Ctrl+Y — Redo");
            EditorAPI.ExecuteCommand("redo");
            return true;
        }

        // Ctrl+C: 复制
        if (ctrl && key == VirtualKey.C)
        {
            Debug.WriteLine("[InputRouter] Hotkey: Ctrl+C — Copy");
            EditorAPI.ExecuteCommand("copy");
            return true;
        }

        // Ctrl+V: 粘贴
        if (ctrl && key == VirtualKey.V)
        {
            Debug.WriteLine("[InputRouter] Hotkey: Ctrl+V — Paste");
            EditorAPI.ExecuteCommand("paste");
            return true;
        }

        // Ctrl+D: 复制选中
        if (ctrl && key == VirtualKey.D)
        {
            Debug.WriteLine("[InputRouter] Hotkey: Ctrl+D — Duplicate");
            EditorAPI.ExecuteCommand("duplicate");
            return true;
        }

        // Ctrl+A: 全选
        if (ctrl && key == VirtualKey.A)
        {
            Debug.WriteLine("[InputRouter] Hotkey: Ctrl+A — Select all");
            EditorAPI.ExecuteCommand("select all");
            return true;
        }

        // F: 聚焦选中实体
        if (!ctrl && key == VirtualKey.F)
        {
            ulong selectedId = EditorAPI.GetSelectedEntity();
            if (selectedId != 0)
            {
                Debug.WriteLine($"[InputRouter] Hotkey: F — Focus entity {selectedId}");
                EditorAPI.ExecuteCommand("focus");
            }
            return true;
        }

        // Delete: 删除选中实体
        if (!ctrl && key == VirtualKey.Delete)
        {
            ulong selectedId = EditorAPI.GetSelectedEntity();
            if (selectedId != 0)
            {
                Debug.WriteLine($"[InputRouter] Hotkey: Delete — Delete entity {selectedId}");
                EditorAPI.DeleteEntity(selectedId);
            }
            return true;
        }

        // Escape: 取消选择
        if (!ctrl && key == VirtualKey.Escape)
        {
            Debug.WriteLine("[InputRouter] Hotkey: Escape — Clear selection");
            EditorAPI.ClearSelection();
            return true;
        }

        return false;
    }

    // ================================================================
    // 键码映射: WinUI3 VirtualKey → 引擎 KeyCode (SDL scancode)
    // ================================================================

    /// 将 WinUI3 VirtualKey 映射到引擎 KeyCode (SDL scancode) 值。
    /// 参考: sdk/include/PrismaEngine/input/InputManager.h 的 KeyCode 枚举。
    /// 返回 0 表示 Unknown（不支持的按键）。
    private static int MapVirtualKey(VirtualKey key) => key switch
    {
        // A-Z (VirtualKey: 65-90 → SDL scancode: 4-29)
        >= VirtualKey.A and <= VirtualKey.Z => (int)key - 61,  // 65-61=4(A), 90-61=29(Z)

        // 数字键 (VirtualKey.Number0=48, Number1=49...)
        VirtualKey.Number0 => 39,
        VirtualKey.Number1 => 30,
        VirtualKey.Number2 => 31,
        VirtualKey.Number3 => 32,
        VirtualKey.Number4 => 33,
        VirtualKey.Number5 => 34,
        VirtualKey.Number6 => 35,
        VirtualKey.Number7 => 36,
        VirtualKey.Number8 => 37,
        VirtualKey.Number9 => 38,

        // 控制键
        VirtualKey.Shift    => 225,  // SDL_SCANCODE_LSHIFT
        VirtualKey.Control  => 224,  // SDL_SCANCODE_LCTRL
        VirtualKey.Menu     => 226,  // SDL_SCANCODE_LALT
        VirtualKey.Space    => 44,
        VirtualKey.Enter    => 40,
        VirtualKey.Escape   => 41,
        VirtualKey.Back     => 42,
        VirtualKey.Tab      => 43,
        VirtualKey.Delete   => 76,

        // 方向键
        VirtualKey.Up       => 82,
        VirtualKey.Down     => 81,
        VirtualKey.Left     => 80,
        VirtualKey.Right    => 79,

        // 功能键
        VirtualKey.F1       => 58,
        VirtualKey.F2       => 59,
        VirtualKey.F3       => 60,
        VirtualKey.F4       => 61,
        VirtualKey.F5       => 62,
        VirtualKey.F6       => 63,
        VirtualKey.F7       => 64,
        VirtualKey.F8       => 65,
        VirtualKey.F9       => 66,
        VirtualKey.F10      => 67,
        VirtualKey.F11      => 68,
        VirtualKey.F12      => 69,

        _ => 0  // KeyCode::Unknown
    };

    /// 判断是否为修饰键 (Shift/Control/Menu)。
    /// 修饰键在 WinUI3 中按住时可能重复触发 KeyDown。
    private static bool IsModifierKey(VirtualKey key) => key switch
    {
        VirtualKey.Shift    => true,
        VirtualKey.Control  => true,
        VirtualKey.Menu     => true,
        _ => false
    };

    // ================================================================
    // 消息投递
    // ================================================================

    /// 将输入事件投递到引擎线程的消息队列。
    /// Action 在引擎线程执行时调用 InputManager 的对应方法。
    private void EnqueueInputEvent(InputEvent evt)
    {
        // 捕获 InputEvent 结构体（值类型，闭包安全）
        _inputQueue.Enqueue(() =>
        {
            // TODO: T16 接入引擎输入系统
            // 需要: EngineInterop.InputManager → SetKeyState/SetMousePosition/SetMouseButtonState
            // 模板:
            //   var im = EngineInterop.GetInputManager();
            //   switch (evt.Type) {
            //       case InputEventType.KeyDown: im.SetKeyState((KeyCode)evt.Key, true); break;
            //       case InputEventType.KeyUp:   im.SetKeyState((KeyCode)evt.Key, false); break;
            //       case InputEventType.MouseMove:
            //           im.SetMousePosition(new vec2(evt.MouseX, evt.MouseY)); break;
            //       case InputEventType.MouseDown:
            //           im.SetMouseButtonState(MouseButton.Right, true); break;
            //       case InputEventType.MouseUp:
            //           im.SetMouseButtonState(MouseButton.Right, false); break;
            //       // MouseWheel 通过摄像机缩放逻辑处理
            //   }

            // 当前: 日志输出（引擎输入接入后移除）
            Debug.WriteLine(
                $"[InputRouter] → Engine: {evt.Type} key={evt.Key} " +
                $"mouse=({evt.MouseX},{evt.MouseY}) delta=({evt.DeltaX},{evt.DeltaY}) " +
                $"wheel={evt.WheelDelta} down={evt.IsDown}");
        });
    }

    // ================================================================
    // IDisposable
    // ================================================================

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        ReleaseAllKeys();
    }
}
