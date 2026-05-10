# Scene Serialization Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Implement full JSONC serialization/deserialization for Component/GameObject/Transform/Scene using Glaze + ComponentRegistry.

**Architecture:** ComponentRegistry maps string → factory function. Each serializable Component defines a nested `Data` struct with `GetData()`/`SetData()`. `glz::generic` bridges component-specific Data to the generic scene JSON format.

**Tech Stack:** C++20, Glaze v7.5.0, GLM, JSONC format

---

### Task 1: ComponentRegistry (New)

**Files:**
- Create: `src/engine/core/ComponentRegistry.h`
- Create: `src/engine/core/ComponentRegistry.cpp`

**Header:**
```cpp
#pragma once
#include "Export.h"
#include "Component.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <typeindex>
#include <functional>

namespace Prisma {

class ENGINE_API ComponentRegistry {
public:
    static ComponentRegistry& Get();

    template<typename T>
    void Register(const char* typeName) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        m_Factories[typeName] = []() { return std::make_shared<T>(); };
        m_TypeNames[std::type_index(typeid(T))] = typeName;
    }

    std::shared_ptr<Component> Create(const std::string& typeName) const;
    std::string_view GetTypeName(const Component& component) const;

private:
    ComponentRegistry() = default;
    std::unordered_map<std::string, std::function<std::shared_ptr<Component>()>> m_Factories;
    std::unordered_map<std::type_index, std::string> m_TypeNames;
};

} // namespace Prisma
```

**Implementation:**
```cpp
#include "ComponentRegistry.h"
#include "Logger.h"

namespace Prisma {

ComponentRegistry& ComponentRegistry::Get() {
    static ComponentRegistry instance;
    return instance;
}

std::shared_ptr<Component> ComponentRegistry::Create(const std::string& typeName) const {
    auto it = m_Factories.find(typeName);
    if (it != m_Factories.end()) {
        return it->second();
    }
    LOG_ERROR("ComponentRegistry", "未知组件类型: {0}", typeName);
    return nullptr;
}

std::string_view ComponentRegistry::GetTypeName(const Component& component) const {
    auto it = m_TypeNames.find(std::type_index(typeid(component)));
    if (it != m_TypeNames.end()) {
        return it->second;
    }
    LOG_WARNING("ComponentRegistry", "未注册的组件类型: {0}", typeid(component).name());
    return "";
}

} // namespace Prisma
```

**Check:** `ComponentRegistry` is self-contained with no dependencies beyond `Component.h`.

---

### Task 2: Component.h — Add Virtual TypeName

**Files:**
- Modify: `src/engine/core/Component.h`

Add pure virtual method:
```cpp
virtual const char* GetComponentTypeName() const = 0;
```

Change nothing else. This forces all Component subclasses to provide a type name.

---

### Task 3: Transform — Add Data Struct + Serialization

**Files:**
- Modify: `src/engine/transform/Transform.h`
- Create: `src/engine/transform/TransformData.h` (or inline in Transform.h)

**Add struct + methods:**
```cpp
struct Data {
    std::array<float, 3> position = {0,0,0};
    std::array<float, 4> rotation = {0,0,0,1}; // quat x,y,z,w
    std::array<float, 3> scale = {1,1,1};
};

Data GetData() const;
void SetData(const Data& d);
```

**Transform is already a Component — it needs GetComponentTypeName():**
```cpp
const char* GetComponentTypeName() const override { return "Transform"; }
```

**Implementation in Transform.cpp:**
```cpp
Transform::Data Transform::GetData() const {
    return {{m_Position.x, m_Position.y, m_Position.z},
            {m_Rotation.x, m_Rotation.y, m_Rotation.z, m_Rotation.w},
            {m_Scale.x, m_Scale.y, m_Scale.z}};
}

void Transform::SetData(const Data& d) {
    SetPosition({d.position[0], d.position[1], d.position[2]});
    SetRotation(Quaternion(d.rotation[0], d.rotation[1], d.rotation[2], d.rotation[3]));
    SetScale({d.scale[0], d.scale[1], d.scale[2]});
}
```

---

### Task 4: GameObject — Add Serialization Support

**Files:**
- Modify: `src/engine/scene/GameObject.h`
- Modify: `src/engine/scene/GameObject.cpp`

**Add to GameObject.h:**
```cpp
#include "glaze/json/generic.hpp"  // for glz::generic

// Data structs (public, for serialization)
struct ComponentEntry {
    std::string type;
    glz::generic data;
};
struct Data {
    std::string name;
    std::array<float, 3> position = {0,0,0};
    std::array<float, 4> rotation = {0,0,0,1};
    std::array<float, 3> scale = {1,1,1};
    std::vector<ComponentEntry> components;
};

// New methods
Data GetData() const;
void SetData(const Data& d);
const std::vector<std::shared_ptr<Component>>& GetComponents() const { return m_Components; }
```

**Implementation in GameObject.cpp:**
```cpp
#include "glaze/json/write.hpp"
#include "glaze/json/read.hpp"
#include "ComponentRegistry.h"

GameObject::Data GameObject::GetData() const {
    Data d;
    d.name = name;
    auto t = GetTransform()->GetData();
    d.position = t.position;
    d.rotation = t.rotation;
    d.scale = t.scale;

    auto& reg = ComponentRegistry::Get();
    for (auto& comp : m_Components) {
        std::string typeName(reg.GetTypeName(*comp));
        if (typeName.empty()) continue;

        // Heuristic: try to find a GetData() method via template
        // We'll use a type-specific approach via the registry
        ComponentEntry entry;
        entry.type = std::move(typeName);
        // data is filled via ComponentDataBridge (see below)
        d.components.push_back(std::move(entry));
    }
    return d;
}
```

**Note:** The polymorphic GetData/SetData challenge is solved via a helper:

```cpp
// ComponentDataBridge.h — template helper for converting Component↔Data
template<typename T>
void ComponentToGeneric(const Component& comp, glz::generic& gen) {
    const auto& typed = static_cast<const T&>(comp);
    auto data = typed.GetData();
    std::string json;
    glz::write_json(data, json);
    glz::read_json(gen, json);
}

template<typename T>
void GenericToComponent(Component& comp, const glz::generic& gen) {
    auto& typed = static_cast<T&>(comp);
    auto json = gen.dump();
    if (json) {
        typename T::Data data;
        glz::read_json(data, *json);
        typed.SetData(data);
    }
}
```

This header is included where needed.

---

### Task 5: Scene — Full Serialization

**Files:**
- Modify: `src/engine/scene/Scene.h`
- Modify: `src/engine/scene/Scene.cpp`

**Scene.h additions:**
```cpp
bool Serialize(const std::string& path) const;
// Deserialize already exists - upgrade it
```

**Scene format (SceneFileData):**
```cpp
struct SceneFileData {
    std::string name;
    struct { std::array<float, 4> projection = {0,1920,0,1080}; } camera;
    std::vector<GameObject::Data> gameObjects;
};
```

**Implementation approach:**

```cpp
bool Scene::Serialize(const std::string& path) const {
    SceneFileData sfd;
    sfd.name = m_Name;
    
    // Camera
    if (m_mainCamera) {
        // Get orthographic projection
        auto ortho = std::dynamic_pointer_cast<Graphic::OrthographicCamera>(m_mainCamera);
        if (ortho) {
            // Read projection values
            // sfd.camera.projection = {left, right, bottom, top};
        }
    }
    
    // GameObjects
    for (auto& go : m_gameObjects) {
        sfd.gameObjects.push_back(go->GetData());
    }
    
    // Write JSON
    auto err = glz::write_file_json(sfd, path, std::string{});
    if (err) {
        LOG_ERROR("Scene", "场景序列化失败: {0}", glz::format_error(err, ""));
        return false;
    }
    return true;
}

bool Scene::Deserialize(const std::string& path) {
    SceneFileData sfd;
    auto err = glz::read_file_jsonc(sfd, path, std::string{});
    if (err) {
        LOG_ERROR("Scene", "场景反序列化失败: {0}", glz::format_error(err, ""));
        return false;
    }
    
    SetName(sfd.name);
    
    // Camera
    auto& p = sfd.camera.projection;
    auto camera = std::make_shared<Graphic::OrthographicCamera>();
    camera->SetProjection(p[0], p[1], p[2], p[3]);
    SetMainCamera(camera);
    
    // GameObjects
    for (auto& god : sfd.gameObjects) {
        auto go = std::make_shared<GameObject>(god.name);
        go->SetData(god);
        AddGameObject(go);
    }
    
    LOG_INFO("Scene", "场景已从 {0} 加载: {1} (含 {2} 个对象)", 
             path, m_Name, m_gameObjects.size());
    return true;
}
```

**Remove:** Old Scene.cpp Deserialize with SpriteConfig etc.

---

### Task 6: SpriteRenderer — Add Data + Registration

**Files:**
- Modify: `src/engine/graphic/SpriteRenderer.h`
- Modify: `src/engine/graphic/SpriteRenderer.cpp`

**Add Data struct:**
```cpp
// SpriteRenderer.h
struct Data {
    std::array<float, 4> color = {1,1,1,1};
    std::array<float, 2> position = {0,0};
    std::array<float, 2> size = {100,100};
    float rotation = 0.0f;
};

const char* GetComponentTypeName() const override { return "SpriteRenderer"; }
Data GetData() const;
void SetData(const Data& d);
```

**Add registration + implementation:**
```cpp
// SpriteRenderer.cpp

// Static registration
namespace {
    bool reg = []() {
        ComponentRegistry::Get().Register<SpriteRenderer>("SpriteRenderer");
        return true;
    }();
}

SpriteRenderer::Data SpriteRenderer::GetData() const {
    return {{m_color.r, m_color.g, m_color.b, m_color.a},
            {m_position.x, m_position.y},
            {m_size.x, m_size.y},
            m_rotation};
}

void SpriteRenderer::SetData(const Data& d) {
    m_color = {d.color[0], d.color[1], d.color[2], d.color[3]};
    m_position = {d.position[0], d.position[1]};
    m_size = {d.size[0], d.size[1]};
    m_rotation = d.rotation;
}
```

---

### Task 7: Scene File Format — Convert to JSONC

**Files:**
- Rename: `projects/Template2D/assets/scenes/2d_test.json` → `2d_test.jsonc`
- Rename: `projects/Template2D/assets/scenes/default.json` → `default.jsonc`

**New 2d_test.jsonc format:**
```jsonc
{
  "name": "2D Test Scene",
  "camera": {
    "projection": [0.0, 1920.0, 0.0, 1080.0]
  },
  "gameObjects": [
    {
      "name": "RedSquare",
      "transform": {
        "position": [200.0, 200.0, 0.0],
        "rotation": [0.0, 0.0, 0.0, 1.0],
        "scale": [100.0, 100.0, 1.0]
      },
      "components": [
        {
          "type": "SpriteRenderer",
          "data": {
            "color": [1.0, 0.2, 0.2, 1.0],
            "position": [200.0, 200.0],
            "size": [100.0, 100.0],
            "rotation": 0.0
          }
        }
      ]
    }
    // ... more GameObjects
  ]
}
```

---

### Task 8: Template2DApp — Adapt to New Scene System

**Files:**
- Modify: `projects/Template2D/src/Template2DApp.h`
- Modify: `projects/Template2D/src/Template2DApp.cpp`

**Remove:**
- `m_CameraComponent` (camera comes from SceneManager)
- Manual `LoadScene` / inline sprite reading
- `m_sprites` (sprites are in Scene GameObjects)
- `CameraComponent.h` include
- All the hand-written Glaze structs (SpriteConfig etc.)

**Add/Change:**
- `OnInitialize()` uses `SceneManager::LoadFromFile("scenes/2d_test.jsonc")`
- `OnRender()` iterates Scene's GameObjects → SpriteRenderers
- Random test sprites added as GameObjects to Scene
- Camera from SceneManager

---

### Execution Summary

| Task | Files | Key Outcome |
|------|-------|-------------|
| 1 | ComponentRegistry.h/.cpp | Type registry system |
| 2 | Component.h | GetComponentTypeName() |
| 3 | Transform.h/.cpp | Transform::Data + serialization |
| 4 | GameObject.h/.cpp | GameObject Data + serialization |
| 5 | Scene.h/.cpp | Full scene I/O with JSONC |
| 6 | SpriteRenderer.h/.cpp | Component serialization example |
| 7 | 2d_test → .jsonc | Scene file format |
| 8 | Template2DApp.h/.cpp | App uses new scene system |

---

### Glaze Meta Specializations Needed

```cpp
// For GameObject::Data
template <>
struct glz::meta<Prisma::ComponentEntry> {
    static constexpr auto value = glz::object(
        "type", &Prisma::ComponentEntry::type,
        "data", &Prisma::ComponentEntry::data
    );
};

template <>
struct glz::meta<Prisma::GameObject::Data> {
    static constexpr auto value = glz::object(
        "name", &Prisma::GameObject::Data::name,
        "transform", &Prisma::GameObject::Data::transform,
        "components", &Prisma::GameObject::Data::components
    );
};
```

Wait — `glz::generic` is already Glaze-serializable by default (no meta needed). So `ComponentEntry` with a `glz::generic` member works without custom meta.

For `GameObject::Data`, we need a `transform` sub-object:

```cpp
struct TransformData {
    std::array<float, 3> position;
    std::array<float, 4> rotation;
    std::array<float, 3> scale;
};
```

Use `TransformData` inline in GameObject::Data instead of three separate arrays. This nests `"transform": { "position": [...], ... }` in JSON.
