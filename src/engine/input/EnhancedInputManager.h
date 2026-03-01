#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

namespace PrismaEngine {
    namespace Input {

        /**
         * @brief 键码定义
         */
        enum class KeyCode : int32_t {
            // 字母键
            A = 0x41, B, C, D, E, F, G, H, I, J, K, L, M,
            N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

            // 数字键
            D0 = 0x30, D1, D2, D3, D4, D5, D6, D7, D8, D9,

            // 功能键
            F1 = 0x70, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

            // 特殊键
            Space = 0x20,
            Enter = 0x0D,
            Escape = 0x1B,
            Tab = 0x09,
            Backspace = 0x08,
            Insert = 0x2D,
            Delete = 0x2E,
            Home = 0x24,
            End = 0x23,
            PageUp = 0x21,
            PageDown = 0x22,

            // 修饰键
            Shift = 0x10,
            Control = 0x11,
            Alt = 0x12,

            // 方向键
            Left = 0x25,
            Up = 0x26,
            Right = 0x27,
            Down = 0x28,

            // 鼠标按钮
            MouseLeft = 0x100,
            MouseRight = 0x101,
            MouseMiddle = 0x102,
            MouseX1 = 0x103,
            MouseX2 = 0x104
        };

        /**
         * @brief 鼠标模式
         */
        enum class MouseMode {
            NORMAL,     // 正常模式：鼠标可见且自由移动
            LOCKED,     // 锁定模式：鼠标隐藏且光标固定在屏幕中心
            HIDDEN,     // 隐藏模式：鼠标隐藏但自由移动
            CAPTURED    // 捕获模式：鼠标被窗口捕获
        };

        /**
         * @brief 输入事件类型
         */
        enum class InputEventType {
            KEY_DOWN,          // 键按下
            KEY_UP,            // 键释放
            KEY_REPEAT,        // 键重复
            MOUSE_MOVED,       // 鼠标移动
            MOUSE_BUTTON_DOWN, // 鼠标按钮按下
            MOUSE_BUTTON_UP,   // 鼠标按钮释放
            MOUSE_SCROLLED,    // 鼠标滚轮
            MOUSE_ENTERED,     // 鼠标进入窗口
            MOUSE_EXITED       // 鼠标离开窗口
        };

        /**
         * @brief 输入事件
         */
        struct InputEvent {
            InputEventType type;
            union {
                struct {
                    KeyCode keyCode;
                    bool shift;
                    bool control;
                    bool alt;
                } key;
                struct {
                    double x;
                    double y;
                    double deltaX;
                    double deltaY;
                } mouse;
                struct {
                    double scrollDelta;
                } scroll;
            };

            InputEvent() : type(InputEventType::KEY_DOWN) {}
        };

        /**
         * @brief 输入事件回调函数类型
         */
        using InputEventCallback = std::function<void(const InputEvent&)>;

        /**
         * @brief 增强的输入管理器
         *
         * 提供鼠标锁定、捕获、输入缓冲等功能
         * 专为 FPS 游戏设计
         */
        class EnhancedInputManager {
        public:
            virtual ~EnhancedInputManager() = default;

            /**
             * @brief 创建增强输入管理器
             */
            static std::unique_ptr<EnhancedInputManager> create();

            // ========== 状态更新 ==========

            /**
             * @brief 每帧更新输入状态
             * @param deltaTime 帧时间
             */
            virtual void update(float deltaTime) = 0;

            /**
             * @brief 处理事件（由平台层调用）
             */
            virtual void handleEvent(const InputEvent& event) = 0;

            // ========== 键盘输入 ==========

            /**
             * @brief 检查键是否按下
             */
            virtual bool isKeyDown(KeyCode key) const = 0;

            /**
             * @brief 检查键是否刚刚按下（本帧按下，上一帧未按下）
             */
            virtual bool isKeyPressed(KeyCode key) const = 0;

            /**
             * @brief 检查键是否刚刚释放（本帧释放，上一帧按下）
             */
            virtual bool isKeyReleased(KeyCode key) const = 0;

            /**
             * @brief 检查键是否重复
             */
            virtual bool isKeyRepeat(KeyCode key) const = 0;

            // ========== 鼠标输入 ==========

            /**
             * @brief 获取鼠标位置（窗口坐标）
             */
            virtual glm::dvec2 getMousePosition() const = 0;

            /**
             * @brief 获取鼠标移动增量（从上一帧到本帧）
             */
            virtual glm::dvec2 getMouseDelta() const = 0;

            /**
             * @brief 重置鼠标增量（通常在处理完输入后调用）
             */
            virtual void resetMouseDelta() = 0;

            /**
             * @brief 检查鼠标按钮是否按下
             */
            virtual bool isMouseButtonDown(int button) const = 0;

            /**
             * @brief 检查鼠标按钮是否刚刚按下
             */
            virtual bool isMouseButtonPressed(int button) const = 0;

            /**
             * @brief 检查鼠标按钮是否刚刚释放
             */
            virtual bool isMouseButtonReleased(int button) const = 0;

            /**
             * @brief 获取鼠标滚轮增量
             */
            virtual double getMouseScrollDelta() const = 0;

            // ========== 鼠标模式控制 ==========

            /**
             * @brief 设置鼠标模式
             */
            virtual void setMouseMode(MouseMode mode) = 0;

            /**
             * @brief 获取当前鼠标模式
             */
            virtual MouseMode getMouseMode() const = 0;

            /**
             * @brief 锁定鼠标（FPS 游戏常用）
             * @param lock 是否锁定
             */
            virtual void setMouseLock(bool lock) = 0;

            /**
             * @brief 检查鼠标是否被锁定
             */
            virtual bool isMouseLocked() const = 0;

            /**
             * @brief 捕获鼠标（鼠标离开窗口时仍然接收事件）
             */
            virtual void captureMouse(bool capture) = 0;

            /**
             * @brief 检查鼠标是否被捕获
             */
            virtual bool isMouseCaptured() const = 0;

            /**
             * @brief 设置鼠标可见性
             */
            virtual void setMouseVisible(bool visible) = 0;

            /**
             * @brief 检查鼠标是否可见
             */
            virtual bool isMouseVisible() const = 0;

            // ========== 事件回调 ==========

            /**
             * @brief 注册键盘事件回调
             */
            virtual void registerKeyEventCallback(const InputEventCallback& callback) = 0;

            /**
             * @brief 注册鼠标事件回调
             */
            virtual void registerMouseEventCallback(const InputEventCallback& callback) = 0;

            /**
             * @brief 移除所有回调
             */
            virtual void clearCallbacks() = 0;

            // ========== 输入缓冲 ==========

            /**
             * @brief 获取文本输入（用于文本框）
             * @return 自上次调用后输入的文本
             */
            virtual std::string getTextInput() = 0;

            /**
             * @brief 设置文本输入光标位置
             */
            virtual void setTextInputCursor(size_t position) = 0;

            /**
             * @brief 开始文本输入
             */
            virtual void startTextInput() = 0;

            /**
             * @brief 停止文本输入
             */
            virtual void stopTextInput() = 0;

            // ========== 输入映射 ==========

            /**
             * @brief 添加输入映射
             * @param actionName 动作名称
             * @param key 绑定的键
             */
            virtual void addKeyMapping(const std::string& actionName, KeyCode key) = 0;

            /**
             * @brief 检查动作是否触发
             */
            virtual bool isActionPressed(const std::string& actionName) const = 0;

            /**
             * @brief 获取动作的模拟值（用于轴向输入，如移动）
             * @param actionName 动作名称（如 "MoveForward"）
             * @param negativeActionName 反向动作名称（如 "MoveBackward"）
             * @return 模拟值 (-1.0 到 1.0)
             */
            virtual float getActionAxis(const std::string& actionName,
                                       const std::string& negativeActionName) const = 0;

            /**
             * @brief 清空所有输入映射
             */
            virtual void clearMappings() = 0;

            // ========== 游戏手柄支持 ==========

            /**
             * @brief 游戏手柄按钮
             */
            enum class GamepadButton : int32_t {
                A = 0,
                B = 1,
                X = 2,
                Y = 3,
                LeftBumper = 4,
                RightBumper = 5,
                Back = 6,
                Start = 7,
                Guide = 8,
                LeftThumb = 9,
                RightThumb = 10,
                DPadUp = 11,
                DPadDown = 12,
                DPadLeft = 13,
                DPadRight = 14
            };

            /**
             * @brief 游戏手柄轴向
             */
            enum class GamepadAxis : int32_t {
                LeftX = 0,
                LeftY = 1,
                RightX = 2,
                RightY = 3,
                LeftTrigger = 4,
                RightTrigger = 5
            };

            /**
             * @brief 检查手柄是否连接
             * @param joystickIndex 手柄索引
             */
            virtual bool isGamepadConnected(int joystickIndex = 0) const = 0;

            /**
             * @brief 检查手柄按钮是否按下
             */
            virtual bool isGamepadButtonDown(GamepadButton button, int joystickIndex = 0) const = 0;

            /**
             * @brief 获取手柄轴向值
             * @return -1.0 到 1.0
             */
            virtual float getGamepadAxis(GamepadAxis axis, int joystickIndex = 0) const = 0;

            /**
             * @brief 设置手柄振动
             * @param leftMotor 左马达强度 (0-1)
             * @param rightMotor 右马达强度 (0-1)
             * @param duration 持续时间（秒）
             */
            virtual void setGamepadVibration(float leftMotor, float rightMotor, float duration, int joystickIndex = 0) = 0;

            // ========== 配置 ==========

            /**
             * @brief 设置鼠标灵敏度
             */
            virtual void setMouseSensitivity(float sensitivity) = 0;

            /**
             * @brief 获取鼠标灵敏度
             */
            virtual float getMouseSensitivity() const = 0;

            /**
             * @brief 设置鼠标平滑因子（用于平滑鼠标移动）
             */
            virtual void setMouseSmoothing(float smoothing) = 0;

            /**
             * @brief 获取平滑后的鼠标增量
             */
            virtual glm::dvec2 getSmoothedMouseDelta() const = 0;

            /**
             * @brief 启用/禁用原始输入（绕过操作系统处理）
             */
            virtual void setRawInput(bool enabled) = 0;

            /**
             * @brief 检查原始输入是否启用
             */
            virtual bool isRawInputEnabled() const = 0;

            // ========== 状态查询 ==========

            /**
             * @brief 检查是否接收到任何输入
             */
            virtual bool hasReceivedInput() const = 0;

            /**
             * @brief 重置输入状态
             */
            virtual void resetInputState() = 0;

            /**
             * @brief 获取输入统计信息
             */
            struct InputStats {
                size_t keyPressCount = 0;
                size_t mouseClickCount = 0;
                double totalMouseDistance = 0.0;
                double totalScrollDistance = 0.0;
            };

            virtual InputStats getStats() const = 0;
        };

    } // namespace Input
} // namespace PrismaEngine
