#pragma once
class GameObject;

class Component {

protected:
    GameObject* m_owner;

public:
    virtual void Update(float deltaTime);
    //Component():m_owner(nullptr) {}
    virtual ~Component() {}
    virtual void Owner(GameObject* owner) { m_owner = owner; }
    virtual void Initialize() {}
    virtual void Shutdown() {}
};