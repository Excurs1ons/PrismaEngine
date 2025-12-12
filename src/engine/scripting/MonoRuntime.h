#pragma once

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/mono-config.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace Engine {
namespace Scripting {

// Mono对象包装器
class MonoObject {
public:
    MonoObject(MonoDomain* domain, ::MonoObject* obj);
    ~MonoObject();

    // 获取原生Mono对象
    ::MonoObject* GetManagedObject() const { return m_monoObject; }

    // 调用方法
    template<typename... Args>
    void CallMethod(const std::string& methodName, Args... args);

    // 获取/设置属性
    template<typename T>
    T GetPropertyValue(const std::string& propertyName);

    template<typename T>
    void SetPropertyValue(const std::string& propertyName, const T& value);

    // 获取字段
    template<typename T>
    T GetFieldValue(const std::string& fieldName);

    template<typename T>
    void SetFieldValue(const std::string& fieldName, const T& value);

private:
    MonoDomain* m_domain;
    ::MonoObject* m_monoObject;
    MonoClass* m_class;
};

// Mono类包装器
class MonoClass {
public:
    MonoClass(MonoDomain* domain, const std::string& namespaceName, const std::string& className);

    // 创建实例
    std::shared_ptr<MonoObject> CreateInstance();

    // 获取方法
    MonoMethod* GetMethod(const std::string& methodName, int paramCount);

    // 获取属性
    MonoProperty* GetProperty(const std::string& propertyName);

    // 获取字段
    MonoField* GetField(const std::string& fieldName);

private:
    MonoDomain* m_domain;
    ::MonoClass* m_monoClass;
};

// Mono方法包装器
class MonoMethod {
public:
    MonoMethod(MonoDomain* domain, ::MonoMethod* method);

    // 调用方法
    template<typename Ret, typename... Args>
    Ret Invoke(MonoObject* obj, Args... args);

    void Invoke(MonoObject* obj);

private:
    MonoDomain* m_domain;
    ::MonoMethod* m_monoMethod;
    void* m_signature;
};

// 脚本组件
class ScriptComponent {
public:
    ScriptComponent(const std::string& scriptPath);
    ~ScriptComponent();

    // 加载脚本
    bool Load();

    // 初始化
    void Initialize();

    // 更新
    void Update(float deltaTime);

    // 销毁
    void Destroy();

    // 设置游戏对象
    void SetGameObject(void* gameObject);

    // 获取脚本实例
    std::shared_ptr<MonoObject> GetInstance() const { return m_instance; }

    // 是否已加载
    bool IsLoaded() const { return m_loaded; }

private:
    std::string m_scriptPath;
    std::shared_ptr<MonoClass> m_scriptClass;
    std::shared_ptr<MonoObject> m_instance;
    void* m_gameObject = nullptr;
    bool m_loaded = false;

    // 缓存常用方法
    MonoMethod* m_awakeMethod = nullptr;
    MonoMethod* m_startMethod = nullptr;
    MonoMethod* m_updateMethod = nullptr;
    MonoMethod* m_onDestroyMethod = nullptr;
};

// Mono运行时管理器
class MonoRuntime {
public:
    static MonoRuntime& GetInstance();

    // 初始化Mono运行时
    bool Initialize();

    // 关闭Mono运行时
    void Shutdown();

    // 加载程序集
    bool LoadAssembly(const std::string& assemblyPath);

    // 创建脚本组件
    std::shared_ptr<ScriptComponent> CreateScript(const std::string& scriptPath);

    // 获取域
    MonoDomain* GetDomain() const { return m_domain; }

    // 注册内部调用函数
    void RegisterInternalCalls();

    // 调用静态方法
    template<typename Ret, typename... Args>
    Ret CallStaticMethod(const std::string& assemblyName,
                        const std::string& namespaceName,
                        const std::string& className,
                        const std::string& methodName,
                        Args... args);

    // 创建C#对象
    template<typename... Args>
    std::shared_ptr<MonoObject> CreateObject(const std::string& namespaceName,
                                            const std::string& className,
                                            Args... args);

    // 垃圾回收
    void CollectGarbage();

private:
    MonoRuntime();
    ~MonoRuntime();

    MonoRuntime(const MonoRuntime&) = delete;
    MonoRuntime& operator=(const MonoRuntime&) = delete;

    // 内部调用函数注册
    void RegisterEngineAPIs();

    // Mono资源
    MonoDomain* m_domain = nullptr;
    MonoAssembly* m_coreAssembly = nullptr;
    MonoImage* m_coreImage = nullptr;

    // 脚本缓存
    std::unordered_map<std::string, std::shared_ptr<MonoClass>> m_classCache;
    std::unordered_map<std::string, MonoAssembly*> m_assemblies;

    // 是否已初始化
    bool m_initialized = false;
};

// 内部调用函数定义
namespace InternalCalls {

// Transform相关
void Transform_SetPosition(void* transform, float x, float y, float z);
void Transform_GetPosition(void* transform, float* x, float* y, float* z);
void Transform_SetRotation(void* transform, float x, float y, float z, float w);
void Transform_GetRotation(void* transform, float* x, float* y, float* z, float* w);
void Transform_SetScale(void* transform, float x, float y, float z);
void Transform_GetScale(void* transform, float* x, float* y, float* z);

// GameObject相关
void* GameObject_Create();
void GameObject_Destroy(void* gameObject);
void* GameObject_AddComponent(void* gameObject, void* componentType);
void* GameObject_GetComponent(void* gameObject, void* componentType);
bool GameObject_HasComponent(void* gameObject, void* componentType);

// Debug相关
void Debug_Log(const char* message);
void Debug_LogWarning(const char* message);
void Debug_LogError(const char* message);

// Time相关
float Time_GetDeltaTime();
float Time_GetTime();

// Input相关
bool Input_GetKey(int keyCode);
bool Input_GetKeyDown(int keyCode);
bool Input_GetKeyUp(int keyCode);
bool Input_GetMouseButton(int button);
float Input_GetMouseX();
float Input_GetMouseY();

// Mathf相关
float Mathf_Sin(float value);
float Mathf_Cos(float value);
float Mathf_Tan(float value);
float Mathf_Abs(float value);
float Mathf_Min(float a, float b);
float Mathf_Max(float a, float b);
float Mathf_Clamp(float value, float min, float max);
float Mathf_Lerp(float a, float b, float t);

} // namespace InternalCalls

} // namespace Scripting
} // namespace Engine