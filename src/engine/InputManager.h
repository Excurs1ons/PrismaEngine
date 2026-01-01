#pragma once
#include "KeyCode.h"
#include "ManagerBase.h"
#include "Platform.h"
#include "Singleton.h"

namespace PrismaEngine::Input {

class InputManager : public ManagerBase<InputManager> {
public:
    bool IsKeyDown(KeyCode key) const;
    bool IsMouseButtonDown(MouseButton button) const;
    void GetMousePosition(float& x, float& y) const;

    void SetPlatform(Platform* platform);
    InputManager() = default;
    ~InputManager();

public:
    bool Initialize() override;
    void Shutdown() override;

private:
    Platform* m_platform = nullptr;
};

} // namespace Engine