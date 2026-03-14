#pragma once
#include "KeyCode.h"
#include "ManagerBase.h"
#include "Platform.h"
#include "Singleton.h"
#include "core/Event.h"
#include <cstring>

namespace Prisma::Input {

class InputManager : public ManagerBase<InputManager> {
public:
    bool IsKeyDown(KeyCode key) const;
    bool IsMouseButtonDown(MouseButton button) const;
    void GetMousePosition(float& x, float& y) const;

    // 事件处理入口
    void OnEvent(Event& e);

    // 强制清理所有状态 (例如窗口失去焦点时)
    void ClearState();

    void SetPlatform(Platform* platform);
    InputManager();
    ~InputManager();

public:
    bool Initialize() override;
    void Shutdown() override;

private:
    Platform* m_platform = nullptr;
    bool m_KeyState[512]; // 够用了，简单、快！
    bool m_MouseState[8];
    float m_MouseX = 0, m_MouseY = 0;
};

} // namespace Prisma::Input