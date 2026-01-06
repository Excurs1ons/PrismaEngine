#pragma once

#include "core/IInputDriver.h"
#include <memory>
#include <functional>
#include <unordered_map>

namespace PrismaEngine::Input {

/// @brief 输入设备类型
enum class InputDriverType {
    Auto = -1,
    Win32 = 0,           // Windows (RawInput + XInput)
    GameActivity = 1,     // Android GameActivity
    SDL3 = 2,            // SDL3 跨平台
};

/// @brief 输入动作类型
enum class InputAction : uint8_t {
    None = 0,
    Press,           // 按下
    Release,         // 释放
    Repeat,          // 重复（长按）
    DoubleClick      // 双击
};

/// @brief 输入动作回调
using InputActionCallback = std::function<void(KeyCode, InputAction)>;

/// @brief 高层输入设备
///
/// 职责：
/// - 使用 IInputDriver 与平台原生输入API交互
/// - 提供输入映射、动作绑定等高级功能
/// - 管理输入状态和事件分发
class InputDevice {
public:
    InputDevice();
    ~InputDevice();

    // ========== 初始化 ==========

    /// @brief 初始化输入设备
    /// @param driverType 驱动类型
    /// @return 是否成功
    bool Initialize(InputDriverType driverType = InputDriverType::Auto);

    /// @brief 关闭输入设备
    void Shutdown();

    /// @brief 检查是否已初始化
    bool IsInitialized() const { return m_initialized.load(); }

    /// @brief 更新输入状态（每帧调用）
    void Update();

    // ========== 键盘查询 ==========

    /// @brief 检查按键是否按下
    bool IsKeyDown(KeyCode key) const;

    /// @brief 检查按键是否刚刚按下
    bool IsKeyJustPressed(KeyCode key) const;

    /// @brief 检查按键是否刚刚释放
    bool IsKeyJustReleased(KeyCode key) const;

    /// @brief 检查任意键是否按下
    bool IsAnyKeyDown() const;

    // ========== 鼠标查询 ==========

    /// @brief 获取鼠标位置
    void GetMousePosition(int& x, int& y) const;

    /// @brief 获取鼠标相对移动
    void GetMouseDelta(int& deltaX, int& deltaY) const;

    /// @brief 检查鼠标按钮是否按下
    bool IsMouseButtonDown(MouseButton button) const;

    /// @brief 检查鼠标按钮是否刚刚按下
    bool IsMouseButtonJustPressed(MouseButton button) const;

    /// @brief 检查鼠标按钮是否刚刚释放
    bool IsMouseButtonJustReleased(MouseButton button) const;

    /// @brief 获取滚轮增量
    int GetMouseWheelDelta() const;

    // ========== 手柄查询 ==========

    /// @brief 获取连接的手柄数量
    uint32_t GetGamepadCount() const;

    /// @brief 检查手柄是否连接
    bool IsGamepadConnected(uint32_t index = 0) const;

    /// @brief 检查手柄按钮是否按下
    bool IsGamepadButtonDown(uint32_t index, GamepadButton button) const;

    /// @brief 获取手柄轴值
    float GetGamepadAxis(uint32_t index, GamepadAxis axis) const;

    /// @brief 设置手柄振动
    void SetGamepadVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration);

    // ========== 输入映射 ==========

    /// @brief 输入动作映射
    struct ActionMapping {
        std::string name;
        KeyCode primaryKey = KeyCode::Unknown;
        KeyCode alternateKey = KeyCode::Unknown;
        GamepadButton gamepadButton = GamepadButton::None;
        float deadzone = 0.0f;
    };

    /// @brief 添加动作映射
    /// @param name 动作名称
    /// @param key 主按键
    /// @param altKey 备用按键
    void AddActionMapping(const std::string& name, KeyCode key, KeyCode altKey = KeyCode::Unknown);

    /// @brief 添加手柄动作映射
    /// @param name 动作名称
    /// @param button 手柄按钮
    void AddActionMapping(const std::string& name, GamepadButton button);

    /// @brief 检查动作是否触发
    bool IsActionPressed(const std::string& name) const;

    /// @brief 检查动作是否刚刚触发
    bool IsActionJustPressed(const std::string& name) const;

    // ========== 文本输入 ==========

    /// @brief 开始文本输入
    void StartTextInput();

    /// @brief 停止文本输入
    void StopTextInput();

    /// @brief 获取输入的文本
    const std::string& GetTextInput() const;

    // ========== 光标控制 ==========

    /// @brief 设置光标可见性
    void SetCursorVisible(bool visible);

    /// @brief 设置光标锁定
    void SetCursorLocked(bool locked);

    /// @brief 获取光标锁定状态
    bool IsCursorLocked() const { return m_cursorLocked; }

private:
    // ========== 驱动创建 ==========
    std::unique_ptr<IInputDriver> CreateDriver(InputDriverType type);

    // ========== 成员变量 ==========

    // 底层驱动
    std::unique_ptr<IInputDriver> m_driver;

    // 动作映射
    std::unordered_map<std::string, ActionMapping> m_actionMappings;

    // 光标状态
    bool m_cursorVisible = true;
    bool m_cursorLocked = false;

    // 状态
    std::atomic<bool> m_initialized{false};
};

} // namespace PrismaEngine::Input
