#include "MonoRuntime.h"
#include "Logger.h"
#include "ECS.h"
#include <iostream>

namespace Engine {
namespace Scripting {

// MonoObject 实现
MonoObject::MonoObject(MonoDomain* domain, ::MonoObject* obj)
    : m_domain(domain), m_monoObject(obj) {
    if (m_monoObject) {
        m_class = mono_object_get_class(m_monoObject);
    }
}

MonoObject::~MonoObject() {
    // Mono对象由GC管理，不需要手动释放
}

// MonoClass 实现
MonoClass::MonoClass(MonoDomain* domain, const std::string& namespaceName, const std::string& className)
    : m_domain(domain) {
    // 从当前域查找类
    m_monoClass = mono_class_from_name_case(
        mono_assembly_get_image(mono_domain_get_assemblies(domain)[0]),
        namespaceName.c_str(),
        className.c_str()
    );
}

std::shared_ptr<MonoObject> MonoClass::CreateInstance() {
    if (!m_monoClass) {
        return nullptr;
    }

    ::MonoObject* obj = mono_object_new(m_domain, m_monoClass);
    if (!obj) {
        return nullptr;
    }

    // 调用构造函数
    mono_runtime_object_init(obj);

    return std::make_shared<Scripting::MonoObject>(m_domain, obj);
}

// MonoMethod 实现
MonoMethod::MonoMethod(MonoDomain* domain, ::MonoMethod* method)
    : m_domain(domain), m_monoMethod(method) {
    if (m_monoMethod) {
        m_signature = mono_method_signature(m_monoMethod);
    }
}

// MonoRuntime 实现
MonoRuntime& MonoRuntime::GetInstance() {
    static MonoRuntime instance;
    return instance;
}

MonoRuntime::MonoRuntime() {
    m_initialized = false;
}

MonoRuntime::~MonoRuntime() {
    Shutdown();
}

bool MonoRuntime::Initialize() {
    if (m_initialized) {
        return true;
    }

    LOG_INFO("MonoRuntime", "初始化Mono运行时");

    // 设置Mono路径
    mono_config_parse(nullptr);

    // 创建JIT域
    m_domain = mono_jit_init("PrismaEngine");
    if (!m_domain) {
        LOG_ERROR("MonoRuntime", "无法创建Mono域");
        return false;
    }

    // 注册内部调用
    RegisterInternalCalls();

    LOG_INFO("MonoRuntime", "Mono运行时初始化成功");
    m_initialized = true;
    return true;
}

void MonoRuntime::Shutdown() {
    if (!m_initialized) {
        return;
    }

    LOG_INFO("MonoRuntime", "关闭Mono运行时");

    // 清理资源
    m_classCache.clear();
    m_assemblies.clear();

    // 关闭JIT
    mono_jit_cleanup(m_domain);
    m_domain = nullptr;
    m_initialized = false;

    LOG_INFO("MonoRuntime", "Mono运行时已关闭");
}

bool MonoRuntime::LoadAssembly(const std::string& assemblyPath) {
    if (!m_initialized) {
        LOG_ERROR("MonoRuntime", "运行时未初始化");
        return false;
    }

    LOG_INFO("MonoRuntime", "加载程序集: {0}", assemblyPath);

    MonoAssembly* assembly = mono_domain_assembly_open(m_domain, assemblyPath.c_str());
    if (!assembly) {
        LOG_ERROR("MonoRuntime", "无法加载程序集: {0}", assemblyPath);
        return false;
    }

    MonoImage* image = mono_assembly_get_image(assembly);
    if (!image) {
        LOG_ERROR("MonoRuntime", "无法获取程序集镜像");
        return false;
    }

    // 缓存程序集
    std::string assemblyName = mono_assembly_get_name(assembly);
    m_assemblies[assemblyName] = assembly;

    // 如果是核心程序集，保存引用
    if (assemblyName == "PrismaEngine.Core" || assemblyName == "PrismaEngine") {
        m_coreAssembly = assembly;
        m_coreImage = image;
    }

    return true;
}

std::shared_ptr<ScriptComponent> MonoRuntime::CreateScript(const std::string& scriptPath) {
    auto script = std::make_shared<ScriptComponent>(scriptPath);
    if (script->Load()) {
        return script;
    }
    return nullptr;
}

void MonoRuntime::RegisterInternalCalls() {
    LOG_DEBUG("MonoRuntime", "注册内部调用函数");

    // Transform
    mono_add_internal_call("PrismaEngine.Transform::SetPosition", (void*)InternalCalls::Transform_SetPosition);
    mono_add_internal_call("PrismaEngine.Transform::GetPosition", (void*)InternalCalls::Transform_GetPosition);
    mono_add_internal_call("PrismaEngine.Transform::SetRotation", (void*)InternalCalls::Transform_SetRotation);
    mono_add_internal_call("PrismaEngine.Transform::GetRotation", (void*)InternalCalls::Transform_GetRotation);
    mono_add_internal_call("PrismaEngine.Transform::SetScale", (void*)InternalCalls::Transform_SetScale);
    mono_add_internal_call("PrismaEngine.Transform::GetScale", (void*)InternalCalls::Transform_GetScale);

    // GameObject
    mono_add_internal_call("PrismaEngine.GameObject::Create", (void*)InternalCalls::GameObject_Create);
    mono_add_internal_call("PrismaEngine.GameObject::Destroy", (void*)InternalCalls::GameObject_Destroy);
    mono_add_internal_call("PrismaEngine.GameObject::AddComponent", (void*)InternalCalls::GameObject_AddComponent);
    mono_add_internal_call("PrismaEngine.GameObject::GetComponent", (void*)InternalCalls::GameObject_GetComponent);
    mono_add_internal_call("PrismaEngine.GameObject::HasComponent", (void*)InternalCalls::GameObject_HasComponent);

    // Debug
    mono_add_internal_call("PrismaEngine.Debug::Log", (void*)InternalCalls::Debug_Log);
    mono_add_internal_call("PrismaEngine.Debug::LogWarning", (void*)InternalCalls::Debug_LogWarning);
    mono_add_internal_call("PrismaEngine.Debug::LogError", (void*)InternalCalls::Debug_LogError);

    // Time
    mono_add_internal_call("PrismaEngine.Time::GetDeltaTime", (void*)InternalCalls::Time_GetDeltaTime);
    mono_add_internal_call("PrismaEngine.Time::GetTime", (void*)InternalCalls::Time_GetTime);

    // Input
    mono_add_internal_call("PrismaEngine.Input::GetKey", (void*)InternalCalls::Input_GetKey);
    mono_add_internal_call("PrismaEngine.Input::GetKeyDown", (void*)InternalCalls::Input_GetKeyDown);
    mono_add_internal_call("PrismaEngine.Input::GetKeyUp", (void*)InternalCalls::Input_GetKeyUp);
    mono_add_internal_call("PrismaEngine.Input::GetMouseButton", (void*)InternalCalls::Input_GetMouseButton);
    mono_add_internal_call("PrismaEngine.Input::GetMouseX", (void*)InternalCalls::Input_GetMouseX);
    mono_add_internal_call("PrismaEngine.Input::GetMouseY", (void*)InternalCalls::Input_GetMouseY);

    // Mathf
    mono_add_internal_call("PrismaEngine.Mathf::Sin", (void*)InternalCalls::Mathf_Sin);
    mono_add_internal_call("PrismaEngine.Mathf::Cos", (void*)InternalCalls::Mathf_Cos);
    mono_add_internal_call("PrismaEngine.Mathf::Tan", (void*)InternalCalls::Mathf_Tan);
    mono_add_internal_call("PrismaEngine.Mathf::Abs", (void*)InternalCalls::Mathf_Abs);
    mono_add_internal_call("PrismaEngine.Mathf::Min", (void*)InternalCalls::Mathf_Min);
    mono_add_internal_call("PrismaEngine.Mathf::Max", (void*)InternalCalls::Mathf_Max);
    mono_add_internal_call("PrismaEngine.Mathf::Clamp", (void*)InternalCalls::Mathf_Clamp);
    mono_add_internal_call("PrismaEngine.Mathf::Lerp", (void*)InternalCalls::Mathf_Lerp);
}

void MonoRuntime::CollectGarbage() {
    if (m_domain) {
        mono_gc_collect(mono_gc_max_generation());
    }
}

// ScriptComponent 实现
ScriptComponent::ScriptComponent(const std::string& scriptPath)
    : m_scriptPath(scriptPath) {
}

ScriptComponent::~ScriptComponent() {
    Destroy();
}

bool ScriptComponent::Load() {
    if (m_loaded) {
        return true;
    }

    auto& runtime = MonoRuntime::GetInstance();

    // 解析脚本路径获取类信息
    // 假设路径格式: "Namespace.ClassName"
    size_t dotPos = m_scriptPath.find_last_of('.');
    if (dotPos == std::string::npos) {
        LOG_ERROR("ScriptComponent", "无效的脚本路径格式: {0}", m_scriptPath);
        return false;
    }

    std::string namespaceName = m_scriptPath.substr(0, dotPos);
    std::string className = m_scriptPath.substr(dotPos + 1);

    // 创建脚本类
    m_scriptClass = std::make_shared<MonoClass>(runtime.GetDomain(), namespaceName, className);
    if (!m_scriptClass) {
        LOG_ERROR("ScriptComponent", "无法创建脚本类: {0}", m_scriptPath);
        return false;
    }

    // 创建实例
    m_instance = m_scriptClass->CreateInstance();
    if (!m_instance) {
        LOG_ERROR("ScriptComponent", "无法创建脚本实例: {0}", m_scriptPath);
        return false;
    }

    // 缓存方法
    m_awakeMethod = m_scriptClass->GetMethod("Awake", 0);
    m_startMethod = m_scriptClass->GetMethod("Start", 0);
    m_updateMethod = m_scriptClass->GetMethod("Update", 1);
    m_onDestroyMethod = m_scriptClass->GetMethod("OnDestroy", 0);

    m_loaded = true;
    LOG_DEBUG("ScriptComponent", "成功加载脚本: {0}", m_scriptPath);
    return true;
}

void ScriptComponent::Initialize() {
    if (m_loaded && m_instance) {
        // 设置游戏对象引用
        // TODO: 通过内部调用设置GameObject字段

        // 调用Awake
        if (m_awakeMethod) {
            m_awakeMethod->Invoke(m_instance.get());
        }
    }
}

void ScriptComponent::Update(float deltaTime) {
    if (m_loaded && m_instance && m_updateMethod) {
        void* args[] = { &deltaTime };
        mono_runtime_invoke(m_updateMethod->GetManagedObject(), m_instance->GetManagedObject(), args, nullptr);
    }
}

void ScriptComponent::Destroy() {
    if (m_loaded && m_instance && m_onDestroyMethod) {
        m_onDestroyMethod->Invoke(m_instance.get());
    }

    m_instance.reset();
    m_scriptClass.reset();
    m_loaded = false;
}

void ScriptComponent::SetGameObject(void* gameObject) {
    m_gameObject = gameObject;
}

// 内部调用函数实现
namespace InternalCalls {

void Transform_SetPosition(void* transform, float x, float y, float z) {
    // TODO: 实现Transform设置位置
}

void Transform_GetPosition(void* transform, float* x, float* y, float* z) {
    // TODO: 实现Transform获取位置
}

void Transform_SetRotation(void* transform, float x, float y, float z, float w) {
    // TODO: 实现Transform设置旋转
}

void Transform_GetRotation(void* transform, float* x, float* y, float* z, float* w) {
    // TODO: 实现Transform获取旋转
}

void Transform_SetScale(void* transform, float x, float y, float z) {
    // TODO: 实现Transform设置缩放
}

void Transform_GetScale(void* transform, float* x, float* y, float* z) {
    // TODO: 实现Transform获取缩放
}

void* GameObject_Create() {
    // TODO: 通过ECS创建实体
    return nullptr;
}

void GameObject_Destroy(void* gameObject) {
    // TODO: 销毁实体
}

void* GameObject_AddComponent(void* gameObject, void* componentType) {
    // TODO: 添加组件
    return nullptr;
}

void* GameObject_GetComponent(void* gameObject, void* componentType) {
    // TODO: 获取组件
    return nullptr;
}

bool GameObject_HasComponent(void* gameObject, void* componentType) {
    // TODO: 检查是否有组件
    return false;
}

void Debug_Log(const char* message) {
    LOG_INFO("Script", "{0}", message ? message : "");
}

void Debug_LogWarning(const char* message) {
    LOG_WARNING("Script", "{0}", message ? message : "");
}

void Debug_LogError(const char* message) {
    LOG_ERROR("Script", "{0}", message ? message : "");
}

float Time_GetDeltaTime() {
    // TODO: 从TimeManager获取
    return 0.016f; // 60 FPS
}

float Time_GetTime() {
    // TODO: 从TimeManager获取
    return 0.0f;
}

bool Input_GetKey(int keyCode) {
    // TODO: 从InputManager获取
    return false;
}

bool Input_GetKeyDown(int keyCode) {
    // TODO: 从InputManager获取
    return false;
}

bool Input_GetKeyUp(int keyCode) {
    // TODO: 从InputManager获取
    return false;
}

bool Input_GetMouseButton(int button) {
    // TODO: 从InputManager获取
    return false;
}

float Input_GetMouseX() {
    // TODO: 从InputManager获取
    return 0.0f;
}

float Input_GetMouseY() {
    // TODO: 从InputManager获取
    return 0.0f;
}

float Mathf_Sin(float value) {
    return sinf(value);
}

float Mathf_Cos(float value) {
    return cosf(value);
}

float Mathf_Tan(float value) {
    return tanf(value);
}

float Mathf_Abs(float value) {
    return fabsf(value);
}

float Mathf_Min(float a, float b) {
    return fminf(a, b);
}

float Mathf_Max(float a, float b) {
    return fmaxf(a, b);
}

float Mathf_Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float Mathf_Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

} // namespace InternalCalls

} // namespace Scripting
} // namespace Engine