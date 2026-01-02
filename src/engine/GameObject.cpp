#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
namespace PrismaEngine{
    GameObject::GameObject(std::string name, std::unique_ptr<Transform> transform) {
        this->name = name;
        if (transform) {
            this->transform = std::move(transform);
        } else {
            this->transform = std::make_unique<Transform>();
            this->transform->SetOwner(this);
            this->transform->Initialize();
        }
    }

    GameObject::GameObject() {
        this->transform = std::make_unique<Transform>();
        this->transform->SetOwner(this);
        this->transform->Initialize();
    }
}